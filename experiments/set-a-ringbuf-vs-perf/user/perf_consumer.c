#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "../bpf/common.h"

static volatile sig_atomic_t stop;
static __u64 lost_events;

static void sig_handler(int sig) { stop = 1; }

static void handle_event(void *ctx, int cpu, void *data, __u32 data_sz)
{
	(void)ctx; (void)cpu; (void)data; (void)data_sz;
}

static void handle_lost(void *ctx, int cpu, __u64 cnt)
{
	lost_events += cnt;
	fprintf(stderr, "Lost %llu events on CPU %d\n", (unsigned long long)cnt, cpu);
}

int main(int argc, char **argv)
{
	struct rusage usage_start, usage_end;
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	printf("perf_consumer: skeleton loading not compiled in this preview\n");
	printf("Expected flow:\n");
	printf("  1. open + load skeleton\n");
	printf("  2. set config_size map\n");
	printf("  3. attach\n");
	printf("  4. create perf_buffer with sample_cb + lost_cb\n");
	printf("  5. getrusage before/after poll\n");
	printf("  6. report lost_events + stats_map\n");

	getrusage(RUSAGE_SELF, &usage_start);
	/* poll loop */
	getrusage(RUSAGE_SELF, &usage_end);

	double user_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
			  (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
	double sys_sec  = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
			  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

	printf("Consumer CPU (placeholder): user=%.4f s  sys=%.4f s\n", user_sec, sys_sec);
	printf("Lost events (userspace): %llu\n", (unsigned long long)lost_events);
	return 0;
}
