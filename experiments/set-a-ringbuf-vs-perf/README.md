# Experiment Set A: Ring Buffer vs Perf Event Array

## Goal

Quantitative comparison of `BPF_MAP_TYPE_RINGBUF` and `BPF_MAP_TYPE_PERF_EVENT_ARRAY` under controlled load.

## Locked Design Decisions

1. Controlled synthetic workload (`workload/syscall_burst.c`)
2. Equal total memory footprint (4 MiB target)
3. Producer-side latency measurement + kernel/userspace drop accounting
4. Out-of-order event tracking

## Directory Layout

```text
set-a-ringbuf-vs-perf/
├── Makefile
├── README.md
├── bpf/
│   ├── common.h
│   ├── ringbuf_producer.bpf.c
│   ├── perf_producer.bpf.c
│   └── vmlinux.h          (generate locally)
├── user/
│   ├── ringbuf_consumer.c
│   └── perf_consumer.c
├── workload/
│   └── syscall_burst.c
├── scripts/
│   └── run_comparison.sh
└── results/
