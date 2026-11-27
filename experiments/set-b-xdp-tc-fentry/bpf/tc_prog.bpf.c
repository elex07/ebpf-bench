// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* tc_prog.bpf.c – Ingress (RX) TC program */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
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

SEC("tc")
int tc_ingress(struct __sk_buff *skb)
{
	__u32 len = skb->len;
	__u32 key = 0;
	__u32 *target_if = bpf_map_lookup_elem(&config_ifindex, &key);

	if (target_if && *target_if != 0 && skb->ifindex != *target_if)
		return TC_ACT_OK;

	update_stats(len, 0);
	return TC_ACT_OK;
}
