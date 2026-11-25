#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "../bpf/common.h"
#include "../bpf/perf_producer.skel.h"

#define TOTAL_BUFFER_BYTES (4 * 1024 * 1024)  /* 4 MiB – must match ringbuf */

static volatile sig_atomic_t stop;
static __u64 events_received;
static __u64 lost_events;

static void sig_handler(int sig) { stop = 1; }

static void handle_event(void *ctx, int cpu, void *data, __u32 data_sz)
{
	(void)ctx; (void)cpu; (void)data; (void)data_sz;
	events_received++;
}

static void handle_lost(void *ctx, int cpu, __u64 cnt)
{
	(void)ctx;
	lost_events += cnt;
	fprintf(stderr, "Lost %llu events on CPU %d\n", (unsigned long long)cnt, cpu);
}

static void print_usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -s, --size <64|512>   Event payload size (default 64)\n"
		"  -d, --duration <sec>  Run duration (default 15)\n"
		"  -h, --help\n", prog);
}

int main(int argc, char **argv)
{
	struct perf_producer_bpf *skel = NULL;
	struct perf_buffer *pb = NULL;
	struct rusage usage_start, usage_end;
	int err = 0, i, ncpus, page_size, per_cpu_pages;
	__u32 size = EVENT_SIZE_64;
	int duration = 15;
	__u32 key = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--size")) {
			if (++i < argc) size = atoi(argv[i]);
		} else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--duration")) {
			if (++i < argc) duration = atoi(argv[i]);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			return 0;
		}
	}

	if (size != 64 && size != 512) {
		fprintf(stderr, "size must be 64 or 512\n");
		return 1;
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	ncpus = libbpf_num_possible_cpus();
	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0) page_size = 4096;
	per_cpu_pages = TOTAL_BUFFER_BYTES / (ncpus * page_size);
	if (per_cpu_pages < 8) per_cpu_pages = 8;

	printf("Perf consumer: %d CPUs, page_size=%d, per_cpu_pages=%d (total ~%d MiB)\n",
	       ncpus, page_size, per_cpu_pages,
	       (per_cpu_pages * ncpus * page_size) / (1024 * 1024));

	skel = perf_producer_bpf__open();
	if (!skel) {
		fprintf(stderr, "Failed to open skeleton\n");
		return 1;
	}

	err = bpf_map__update_elem(skel->maps.config_size, &key, sizeof(key),
				   &size, sizeof(size), BPF_ANY);
	if (err) {
		fprintf(stderr, "Failed to set config_size: %d\n", err);
		goto cleanup;
	}

	err = perf_producer_bpf__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load skeleton: %d\n", err);
		goto cleanup;
	}

	err = perf_producer_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach: %d\n", err);
		goto cleanup;
	}

	pb = perf_buffer__new(bpf_map__fd(skel->maps.events), per_cpu_pages,
			      handle_event, handle_lost, NULL, NULL);
	if (!pb) {
		fprintf(stderr, "Failed to create perf buffer\n");
		err = -1;
		goto cleanup;
	}

	printf("Perf consumer started (size=%u, duration=%ds)\n", size, duration);
	fflush(stdout);

	getrusage(RUSAGE_SELF, &usage_start);

	time_t end = time(NULL) + duration;
	while (!stop && time(NULL) < end) {
		err = perf_buffer__poll(pb, 100);
		if (err == -EINTR) break;
		if (err < 0) {
			fprintf(stderr, "perf_buffer__poll error: %d\n", err);
			break;
		}
	}

	getrusage(RUSAGE_SELF, &usage_end);

	struct bpf_stats *values = calloc(ncpus, sizeof(*values));
	struct bpf_stats total = {};

	if (values) {
		err = bpf_map__lookup_elem(skel->maps.stats_map, &key, sizeof(key),
					   values, ncpus * sizeof(*values), 0);
		if (!err) {
			for (i = 0; i < ncpus; i++) {
				total.events_submitted += values[i].events_submitted;
				total.events_dropped   += values[i].events_dropped;
				total.output_failures  += values[i].output_failures;
			}
		}
		free(values);
	}

	double user_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
			  (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
	double sys_sec  = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
			  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

	printf("\n=== Perf Consumer Results ===\n");
	printf("events_received     : %llu\n", (unsigned long long)events_received);
	printf("lost_events         : %llu\n", (unsigned long long)lost_events);
	printf("events_submitted    : %llu\n", (unsigned long long)total.events_submitted);
	printf("events_dropped      : %llu\n", (unsigned long long)total.events_dropped);
	printf("output_failures     : %llu\n", (unsigned long long)total.output_failures);
	printf("consumer_user_sec   : %.4f\n", user_sec);
	printf("consumer_sys_sec    : %.4f\n", sys_sec);
	printf("consumer_total_sec  : %.4f\n", user_sec + sys_sec);

cleanup:
	perf_buffer__free(pb);
	perf_producer_bpf__destroy(skel);
	return err < 0 ? -err : 0;
}
