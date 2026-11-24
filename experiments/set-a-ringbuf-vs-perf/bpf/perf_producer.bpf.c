// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* perf_producer.bpf.c */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "common.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(__u32));
	__uint(value_size, sizeof(__u32));
} events SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct bpf_stats);
} stats_map SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, __u32);
} config_size SEC(".maps");

static __always_inline void update_stats(int dropped, int output_fail)
{
	__u32 key = STATS_MAP_KEY;
	struct bpf_stats *s = bpf_map_lookup_elem(&stats_map, &key);
	if (!s)
		return;
	s->events_submitted++;
	if (dropped)
		s->events_dropped++;
	if (output_fail)
		s->output_failures++;
}

SEC("tp/syscalls/sys_enter_getpid")
int handle_getpid(void *ctx)
{
	__u32 key = 0;
	__u32 *psz = bpf_map_lookup_elem(&config_size, &key);
	__u32 size = psz ? *psz : EVENT_SIZE_64;
	int ret;

	if (size == EVENT_SIZE_512) {
		struct event_512 e = {};
		e.hdr.timestamp_ns = bpf_ktime_get_ns();
		e.hdr.cpu = bpf_get_smp_processor_id();
		e.hdr.pid = bpf_get_current_pid_tgid() >> 32;
		e.hdr.size = EVENT_SIZE_512;
		e.hdr.seq = 0;
		__builtin_memset(e.payload, 0xAB, sizeof(e.payload));
		ret = bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
		if (ret) {
			update_stats(1, 1);
			return 0;
		}
		update_stats(0, 0);
	} else {
		struct event_64 e = {};
		e.hdr.timestamp_ns = bpf_ktime_get_ns();
		e.hdr.cpu = bpf_get_smp_processor_id();
		e.hdr.pid = bpf_get_current_pid_tgid() >> 32;
		e.hdr.size = EVENT_SIZE_64;
		e.hdr.seq = 0;
		__builtin_memset(e.payload, 0xCD, sizeof(e.payload));
		ret = bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
		if (ret) {
			update_stats(1, 1);
			return 0;
		}
		update_stats(0, 0);
	}
	return 0;
}
