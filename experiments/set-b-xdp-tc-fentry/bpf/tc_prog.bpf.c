// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* tc_prog.bpf.c
 * TC (Traffic Control) program for Experiment Set B
 * Attaches at the qdisc layer (later than XDP).
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

/* TC ingress hook */
SEC("tc")
int tc_ingress(struct __sk_buff *skb)
{
	__u32 len = skb->len;

	/* Count every packet that reaches TC */
	update_stats(len, 0);

	/* Continue normal processing */
	return TC_ACT_OK;
}
