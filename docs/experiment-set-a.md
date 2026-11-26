# Experiment Set A — Ring Buffer vs Perf Event Array

## Goal

Compare `BPF_MAP_TYPE_RINGBUF` and `BPF_MAP_TYPE_PERF_EVENT_ARRAY` under controlled conditions.

## Locked Design Decisions

1. **Controlled synthetic workload**  
   `workload/syscall_burst.c` generates events at precise rates on 1 or 4 cores.

2. **Equal total memory footprint**  
   Both mechanisms are configured with the same total kernel buffer size.

3. **Producer-side measurement**  
   Syscall latency (p50 / p99 / p99.9) is measured with:
   - No eBPF (baseline)
   - Ring Buffer attached
   - Perf Event Array attached

4. **Proper drop accounting**  
   Kernel-side atomic counters + userspace lost-sample callbacks.

## Controlled Variables

| Parameter | Values |
|-----------|--------|
| Mechanism | Baseline / RingBuf / Perf Event Array |
| Payload size | 64 bytes / 512 bytes |
| Producer scale | 1 core / 4 cores |
| Consumer | Dedicated core |
| Metrics | Producer latency, drop %, consumer CPU, reordering |

## Directory Layout

```text
experiments/set-a-ringbuf-vs-perf/
├── bpf/
│   ├── common.h
│   ├── ringbuf_producer.bpf.c
│   └── perf_producer.bpf.c
├── user/
│   ├── ringbuf_consumer.c
│   └── perf_consumer.c
├── workload/
│   └── syscall_burst.c
├── scripts/
│   └── run_comparison.sh
└── results/
```

## Key Metrics Collected

- Producer syscall latency distribution
- Kernel drop rate
- Consumer user + system CPU time
- Event reordering rate (Perf only)

## Recommended Smoke-Test Sequence

```bash
cd ebpf-bench/experiments/set-a-ringbuf-vs-perf

# Generate vmlinux.h (required for CO-RE)
bpftool btf dump file /sys/kernel/btf/vmlinux format c > bpf/vmlinux.h

# Build
make all

# Run full comparison (must be root)
sudo ./scripts/run_comparison.sh

```


