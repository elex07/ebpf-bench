// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* fentry_prog.bpf.c
 * fentry program for Experiment Set B
 * Attaches to a network-related kernel function for comparison
 * against XDP and TC.
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "common.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

/* Per-CPU statistics */
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct bpf_stats);
} stats_map SEC(".maps");

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

/*
 * fentry on a common transmit path function.
 * Note: exact symbol may vary by kernel; __dev_queue_xmit is widely available.
 */
SEC("fentry/__dev_queue_xmit")
int BPF_PROG(fentry_dev_queue_xmit, struct sk_buff *skb)
{
	__u32 len = 0;

	if (skb)
		len = skb->len;

	update_stats(len, 0);
	return 0;
}
