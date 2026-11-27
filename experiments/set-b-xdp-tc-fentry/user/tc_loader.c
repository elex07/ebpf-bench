#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <net/if.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "../bpf/common.h"
#include "../bpf/tc_prog.skel.h"

static volatile sig_atomic_t stop;
static void sig_handler(int sig) { stop = 1; }

static void print_usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s -i <ifname> [-d duration_sec]\n"
		"  -i, --interface   Network interface (required)\n"
		"  -d, --duration    Run duration in seconds (default 15)\n", prog);
}

int main(int argc, char **argv)
{
	struct tc_prog_bpf *skel = NULL;
	char ifname[IF_NAMESIZE] = "";
	int ifindex = 0, duration = 15, err = 0, i, ncpus;
	__u32 key = 0, cfg_ifindex = 0;
	struct bpf_prog_info info = {};
	__u32 info_len = sizeof(info);
	__u64 run_time_ns_start = 0, run_cnt_start = 0;
	__u64 run_time_ns_end = 0, run_cnt_end = 0;

	for (i = 1; i < argc; i++) {
		if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "--interface")) && i + 1 < argc)
			strncpy(ifname, argv[++i], IF_NAMESIZE - 1);
		else if ((!strcmp(argv[i], "-d") || !strcmp(argv[i], "--duration")) && i + 1 < argc)
			duration = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			return 0;
		}
	}

	if (ifname[0] == '\0') {
		fprintf(stderr, "Interface (-i) is required\n");
		return 1;
	}

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "Failed to resolve interface %s\n", ifname);
		return 1;
	}
	cfg_ifindex = ifindex;

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	err = bpf_enable_stats(BPF_STATS_RUN_TIME);
	if (err)
		fprintf(stderr, "Warning: bpf_enable_stats failed (%d)\n", err);

	skel = tc_prog_bpf__open();
	if (!skel) {
		fprintf(stderr, "Failed to open TC skeleton\n");
		return 1;
	}

	err = bpf_map__update_elem(skel->maps.config_ifindex, &key, sizeof(key),
				   &cfg_ifindex, sizeof(cfg_ifindex), BPF_ANY);
	if (err)
		fprintf(stderr, "Warning: failed to set config_ifindex\n");

	err = tc_prog_bpf__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load TC skeleton: %d\n", err);
		goto cleanup;
	}

	DECLARE_LIBBPF_OPTS(bpf_tc_hook, hook, .ifindex = ifindex, .attach_point = BPF_TC_INGRESS);
	DECLARE_LIBBPF_OPTS(bpf_tc_opts, opts, .handle = 1, .priority = 1);

	err = bpf_tc_hook_create(&hook);
	if (err && err != -EEXIST) {
		fprintf(stderr, "Failed to create TC hook: %d\n", err);
		goto cleanup;
	}

	opts.prog_fd = bpf_program__fd(skel->progs.tc_ingress);
	err = bpf_tc_attach(&hook, &opts);
	if (err) {
		fprintf(stderr, "Failed to attach TC program: %d\n", err);
		goto cleanup;
	}

	memset(&info, 0, sizeof(info));
	info_len = sizeof(info);
	if (bpf_obj_get_info_by_fd(bpf_program__fd(skel->progs.tc_ingress), &info, &info_len) == 0) {
		run_time_ns_start = info.run_time_ns;
		run_cnt_start    = info.run_cnt;
	}

	printf("TC attached on %s (ifindex %d), duration=%ds\n", ifname, ifindex, duration);
	fflush(stdout);

	time_t end = time(NULL) + duration;
	while (!stop && time(NULL) < end)
		sleep(1);

	memset(&info, 0, sizeof(info));
	info_len = sizeof(info);
	if (bpf_obj_get_info_by_fd(bpf_program__fd(skel->progs.tc_ingress), &info, &info_len) == 0) {
		run_time_ns_end = info.run_time_ns;
		run_cnt_end    = info.run_cnt;
	}

	opts.flags = opts.prog_fd = opts.prog_id = opts.handle = opts.priority = 0;
	bpf_tc_detach(&hook, &opts);

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

	__u64 delta_time = run_time_ns_end - run_time_ns_start;
	__u64 delta_cnt  = run_cnt_end - run_cnt_start;
	double ns_per_pkt = (delta_cnt > 0) ? (double)delta_time / delta_cnt : 0.0;

	printf("\n=== TC Results ===\n");
	printf("packets_seen       : %llu\n", (unsigned long long)total.packets_seen);
	printf("packets_dropped    : %llu\n", (unsigned long long)total.packets_dropped);
	printf("bytes_seen         : %llu\n", (unsigned long long)total.bytes_seen);
	printf("bpf_run_cnt        : %llu\n", (unsigned long long)delta_cnt);
	printf("bpf_run_time_ns    : %llu\n", (unsigned long long)delta_time);
	printf("bpf_ns_per_packet  : %.2f\n", ns_per_pkt);

cleanup:
	tc_prog_bpf__destroy(skel);
	return err < 0 ? -err : 0;
}
