/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
#ifndef __COMMON_H
#define __COMMON_H

/*
 * Shared definitions for Ring Buffer vs Perf Event Array comparison.
 *
 * Design constraints (locked):
 * - Support 64-byte and 512-byte event payloads
 * - Equal total memory footprint between mechanisms
 * - Kernel-side drop accounting via atomic counters
 */

#define MAX_CPUS 64

/* Event payload sizes we will sweep */
#define EVENT_SIZE_64   64
#define EVENT_SIZE_512  512

/*
 * Common event header.
 * Keep it cache-line friendly.
 */
struct event_hdr {
    __u64 timestamp_ns;     /* timestamp from BPF */
    __u32 cpu;              /* smp_processor_id() */
    __u32 pid;
    __u32 size;             /* actual payload size used for this event */
    __u32 seq;              /* sequence number (for ordering checks) */
};

/*
 * 64-byte event (1 cache line)
 */
struct event_64 {
    struct event_hdr hdr;
    char payload[EVENT_SIZE_64 - sizeof(struct event_hdr)];
};

/*
 * 512-byte event
 */
struct event_512 {
    struct event_hdr hdr;
    char payload[EVENT_SIZE_512 - sizeof(struct event_hdr)];
};

/*
 * Kernel-side statistics
 */
struct bpf_stats {
    __u64 events_submitted;
    __u64 events_dropped;
    __u64 reserve_failures;
    __u64 output_failures;
};

#define STATS_MAP_KEY 0

#endif /* __COMMON_H */
