/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
#ifndef __SET_B_COMMON_H
#define __SET_B_COMMON_H

/*
 * Shared definitions for Experiment Set B
 * XDP vs TC vs fentry/kprobe – all on the ingress (RX) path
 */

#define MAX_CPUS 64

/* TC action codes (avoid including linux/pkt_cls.h with vmlinux.h) */
#define TC_ACT_OK   0
#define TC_ACT_SHOT 2

/* Simple event recorded on every packet / hook invocation */
/* Simple per-packet / per-invocation record */
struct event_hdr {
	__u64 timestamp_ns;   /* bpf_ktime_get_ns() */
	__u32 cpu;
	__u32 ifindex;
	__u32 len;            /* packet length */
	__u32 seq;
};

_Static_assert(sizeof(struct event_hdr) == 24, "event_hdr must be 24 bytes");

/* Kernel-side statistics (per-CPU) */
struct bpf_stats {
	__u64 packets_seen;
	__u64 packets_dropped;     /* XDP_DROP / TC drop / etc. */
	__u64 bytes_seen;
	__u64 errors;
};

#define STATS_MAP_KEY 0

#endif /* __SET_B_COMMON_H */
