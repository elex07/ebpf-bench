// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* ringbuf_producer.bpf.c */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "common.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 1 << 22);	/* 4 MiB */
} rb SEC(".maps");

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

static __always_inline void update_stats(int dropped, int reserve_fail)
{
	__u32 key = STATS_MAP_KEY;
	struct bpf_stats *s = bpf_map_lookup_elem(&stats_map, &key);
	if (!s)
		return;
	s->events_submitted++;
	if (dropped)
		s->events_dropped++;
	if (reserve_fail)
		s->reserve_failures++;
}

SEC("tp/syscalls/sys_enter_getpid")
int handle_getpid(void *ctx)
{
	__u32 key = 0;
	__u32 *psz = bpf_map_lookup_elem(&config_size, &key);
	__u32 size = psz ? *psz : EVENT_SIZE_64;

	if (size == EVENT_SIZE_512) {
		struct event_512 *e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
		if (!e) {
			update_stats(1, 1);
			return 0;
		}
		e->hdr.timestamp_ns = bpf_ktime_get_ns();
		e->hdr.cpu = bpf_get_smp_processor_id();
		e->hdr.pid = bpf_get_current_pid_tgid() >> 32;
		e->hdr.size = EVENT_SIZE_512;
		e->hdr.seq = 0;
		__builtin_memset(e->payload, 0xAB, sizeof(e->payload));
		bpf_ringbuf_submit(e, 0);
		update_stats(0, 0);
	} else {
		struct event_64 *e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
		if (!e) {
			update_stats(1, 1);
			return 0;
		}
		e->hdr.timestamp_ns = bpf_ktime_get_ns();
		e->hdr.cpu = bpf_get_smp_processor_id();
		e->hdr.pid = bpf_get_current_pid_tgid() >> 32;
		e->hdr.size = EVENT_SIZE_64;
		e->hdr.seq = 0;
		__builtin_memset(e->payload, 0xCD, sizeof(e->payload));
		bpf_ringbuf_submit(e, 0);
		update_stats(0, 0);
	}
	return 0;
}
