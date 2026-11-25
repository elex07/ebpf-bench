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
#include "../bpf/ringbuf_producer.skel.h"

static volatile sig_atomic_t stop;
static __u64 events_received;

static void sig_handler(int sig) { stop = 1; }

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	(void)ctx; (void)data; (void)data_sz;
	events_received++;
	return 0;
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
	struct ringbuf_producer_bpf *skel = NULL;
	struct ring_buffer *rb = NULL;
	struct rusage usage_start, usage_end;
	int err = 0, i, ncpus;
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

	skel = ringbuf_producer_bpf__open();
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

	err = ringbuf_producer_bpf__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load skeleton: %d\n", err);
		goto cleanup;
	}

	err = ringbuf_producer_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach: %d\n", err);
		goto cleanup;
	}

	rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
	if (!rb) {
		fprintf(stderr, "Failed to create ring buffer\n");
		err = -1;
		goto cleanup;
	}

	printf("RingBuf consumer started (size=%u, duration=%ds)\n", size, duration);
	fflush(stdout);

	getrusage(RUSAGE_SELF, &usage_start);

	time_t end = time(NULL) + duration;
	while (!stop && time(NULL) < end) {
		err = ring_buffer__poll(rb, 100);
		if (err == -EINTR) break;
		if (err < 0) {
			fprintf(stderr, "ring_buffer__poll error: %d\n", err);
			break;
		}
	}

	getrusage(RUSAGE_SELF, &usage_end);

	ncpus = libbpf_num_possible_cpus();
	struct bpf_stats *values = calloc(ncpus, sizeof(*values));
	struct bpf_stats total = {};

	if (values) {
		err = bpf_map__lookup_elem(skel->maps.stats_map, &key, sizeof(key),
					   values, ncpus * sizeof(*values), 0);
		if (!err) {
			for (i = 0; i < ncpus; i++) {
				total.events_submitted += values[i].events_submitted;
				total.events_dropped   += values[i].events_dropped;
				total.reserve_failures += values[i].reserve_failures;
			}
		}
		free(values);
	}

	double user_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
			  (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
	double sys_sec  = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
			  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

	printf("\n=== RingBuf Consumer Results ===\n");
	printf("events_received     : %llu\n", (unsigned long long)events_received);
	printf("events_submitted    : %llu\n", (unsigned long long)total.events_submitted);
	printf("events_dropped      : %llu\n", (unsigned long long)total.events_dropped);
	printf("reserve_failures    : %llu\n", (unsigned long long)total.reserve_failures);
	printf("consumer_user_sec   : %.4f\n", user_sec);
	printf("consumer_sys_sec    : %.4f\n", sys_sec);
	printf("consumer_total_sec  : %.4f\n", user_sec + sys_sec);

cleanup:
	ring_buffer__free(rb);
	ringbuf_producer_bpf__destroy(skel);
	return err < 0 ? -err : 0;
}
