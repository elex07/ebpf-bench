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

static void sig_handler(int sig) { stop = 1; }

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	(void)ctx; (void)data; (void)data_sz;
	return 0;
}

int main(int argc, char **argv)
{
	struct rusage usage_start, usage_end;
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	printf("ringbuf_consumer: skeleton loading not compiled in this preview\n");
	printf("Expected flow:\n");
	printf("  1. open + load skeleton\n");
	printf("  2. set config_size map (64 or 512)\n");
	printf("  3. attach to sys_enter_getpid\n");
	printf("  4. create ring_buffer with handle_event callback\n");
	printf("  5. getrusage before poll loop\n");
	printf("  6. poll until stopped\n");
	printf("  7. getrusage after → user+sys CPU\n");
	printf("  8. read stats_map\n");

	getrusage(RUSAGE_SELF, &usage_start);
	/* poll loop would go here */
	getrusage(RUSAGE_SELF, &usage_end);

	double user_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
			  (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
	double sys_sec  = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
			  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

	printf("Consumer CPU (placeholder): user=%.4f s  sys=%.4f s\n", user_sec, sys_sec);
	return 0;
}
