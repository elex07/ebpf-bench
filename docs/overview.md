# Project Overview

## Research Goal

This project performs a controlled, quantitative comparison of two primary eBPF data transfer mechanisms:

- `BPF_MAP_TYPE_RINGBUF`
- `BPF_MAP_TYPE_PERF_EVENT_ARRAY`

The focus is on realistic high-rate scenarios and on measuring both **consumer-side** and **producer-side** costs.

## Why This Matters

Many existing comparisons only report consumer CPU or simple throughput.  
In production systems the more important cost is often the slowdown of the application thread that triggers the eBPF program (producer-side overhead).

This work explicitly measures:

- Producer syscall latency (p50, p99, p99.9)
- Kernel-side event drops
- Consumer CPU usage
- Ordering differences under multi-core load

## High-Level Methodology

1. **Noise Control First**  
   Establish a clean zero-load baseline with strong CPU isolation before attaching any eBPF programs.

2. **Controlled Workload**  
   Use a synthetic multi-core generator (`syscall_burst`) that produces events at precise target rates.

3. **Fair Comparison**  
   Keep total kernel buffer memory equal between Ring Buffer and Perf Event Array.

4. **Rich Metrics**  
   Capture producer latency, drop rates, consumer CPU, and reordering.

## Experiment Roadmap

| Experiment | Focus |
|------------|-------|
| Set A | Ring Buffer vs Perf Event Array |
| Set B | XDP vs TC vs fentry/kprobe (planned) |

## Intended Audience

- Researchers working on eBPF performance
- Engineers evaluating observability / tracing overhead
- Anyone who needs a reproducible baseline for systems measurements
