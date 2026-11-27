// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* xdp_prog.bpf.c – Ingress (RX) XDP program */
/* xdp_prog.bpf.c
 * XDP program for Experiment Set B
 * Attaches at the earliest point in the network stack (driver level).
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include "common.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

/* Per-CPU statistics */
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
} config_ifindex SEC(".maps");

static __always_inline void update_stats(__u32 len, int drop)
{
	__u32 key = STATS_MAP_KEY;
	struct bpf_stats *s = bpf_map_lookup_elem(&stats_map, &key);
	if (!s)
		return;

	s->packets_seen++;
	s->bytes_seen += len;
	if (drop)
		s->packets_dropped++;
}

SEC("xdp")
int xdp_pass(struct xdp_md *ctx)
{
	void *data     = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;
	__u32 len = (__u32)(data_end - data);
	__u32 key = 0;
	__u32 *target_if = bpf_map_lookup_elem(&config_ifindex, &key);

	if (target_if && *target_if != 0 && ctx->ingress_ifindex != *target_if)
		return XDP_PASS;

	/* Basic bounds check */
	if (data + 14 > data_end) {		/* minimum Ethernet header */
		update_stats(0, 1);
		return XDP_DROP;
	}

	/* Count the packet and pass it up the stack */
	update_stats(len, 0);

	return XDP_PASS;
}
