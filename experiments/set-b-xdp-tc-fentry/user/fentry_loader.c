#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "../bpf/common.h"
#include "../bpf/fentry_prog.skel.h"

static volatile sig_atomic_t stop;
static void sig_handler(int sig) { stop = 1; }

static void print_usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [-d duration_sec]\n"
		"  -d, --duration    Run duration in seconds (default 15)\n", prog);
}

int main(int argc, char **argv)
{
	struct fentry_prog_bpf *skel = NULL;
	struct rusage usage_start, usage_end;
	int duration = 15, err = 0, i, ncpus;
	__u32 key = 0;

	for (i = 1; i < argc; i++) {
		if ((!strcmp(argv[i], "-d") || !strcmp(argv[i], "--duration")) && i + 1 < argc)
			duration = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			return 0;
		}
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	skel = fentry_prog_bpf__open();
	if (!skel) {
		fprintf(stderr, "Failed to open fentry skeleton\n");
		return 1;
	}

	err = fentry_prog_bpf__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load fentry skeleton: %d\n", err);
		goto cleanup;
	}

	err = fentry_prog_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach fentry program: %d\n", err);
		goto cleanup;
	}

	printf("fentry program attached, duration=%ds\n", duration);
	fflush(stdout);

	getrusage(RUSAGE_SELF, &usage_start);
	time_t end = time(NULL) + duration;
	while (!stop && time(NULL) < end)
		sleep(1);
	getrusage(RUSAGE_SELF, &usage_end);

	ncpus = libbpf_num_possible_cpus();
	struct bpf_stats *values = calloc(ncpus, sizeof(*values));
	struct bpf_stats total = {};

	if (values) {
		err = bpf_map__lookup_elem(skel->maps.stats_map, &key, sizeof(key),
					   values, ncpus * sizeof(*values), 0);
		if (!err) {
			for (i = 0; i < ncpus; i++) {
				total.packets_seen    += values[i].packets_seen;
				total.packets_dropped += values[i].packets_dropped;
				total.bytes_seen      += values[i].bytes_seen;
				total.errors          += values[i].errors;
			}
		}
		free(values);
	}

	double user_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
			  (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
	double sys_sec  = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
			  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

	printf("\n=== fentry Results ===\n");
	printf("packets_seen      : %llu\n", (unsigned long long)total.packets_seen);
	printf("packets_dropped   : %llu\n", (unsigned long long)total.packets_dropped);
	printf("bytes_seen        : %llu\n", (unsigned long long)total.bytes_seen);
	printf("errors            : %llu\n", (unsigned long long)total.errors);
	printf("consumer_user_sec : %.4f\n", user_sec);
	printf("consumer_sys_sec  : %.4f\n", sys_sec);
	printf("consumer_total_sec: %.4f\n", user_sec + sys_sec);

cleanup:
	fentry_prog_bpf__destroy(skel);
	return err < 0 ? -err : 0;
}
