# Interpreting Results

## Baseline Jitter Results

Focus on these fields from `02-baseline-jitter.sh`:

| Field | Meaning |
|-------|---------|
| `p99.9_ns` | 99.9th percentile timer delta |
| `p99.99_ns` | 99.99th percentile (tail) |
| `mean_ns` | Average sampling interval |

**Rule of thumb**

- Stable and low p99.9 → good isolation
- Large spikes or high run-to-run variation → remaining noise sources

## Experiment Set A Results

When comparing Ring Buffer vs Perf Event Array, examine:

| Metric | What it tells you |
|--------|-------------------|
| Producer p99 / p99.9 latency | Cost imposed on the application thread |
| Kernel drop % | How often events are lost under load |
| Consumer CPU (user + sys) | Cost of draining the buffer |
| Reordering rate | Only relevant for Perf Event Array under multi-core |

## General Advice

- Always compare against the **No eBPF** baseline
- Prefer percentile numbers over averages
- Look at multiple runs for stability
- Document exact buffer sizes and core pinning used
