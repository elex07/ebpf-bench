// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* fentry_prog.bpf.c
 * Ingress (RX) path – attached to __netif_receive_skb_core
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "common.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

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

SEC("fentry/__netif_receive_skb_core")
int BPF_PROG(fentry_netif_receive_skb_core, struct sk_buff *skb, bool *pfmemalloc)
{
	__u32 len = 0;
	__u32 key = 0;
	__u32 *target_if;
	__u32 ifindex = 0;

	if (!skb)
		return 0;

	len = skb->len;

	target_if = bpf_map_lookup_elem(&config_ifindex, &key);
	if (target_if && *target_if != 0) {
		if (skb->dev)
			ifindex = skb->dev->ifindex;
		if (ifindex != *target_if)
			return 0;
	}

	update_stats(len, 0);
	return 0;
}
