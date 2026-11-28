# Experiment Set B – Sample Results

These files illustrate the expected output format after running:

```bash
sudo ./scripts/run_comparison.sh
```

| File | Description |
| --- | --- |
| summary-*.csv | Aggregated metrics across all mechanisms |
| xdp-*.txt / tc-*.txt / fentry-*.txt | Per-run detailed logs |
| *-perf-*.txt | perf stat -C 2 hardware counters |

### Interpretation notes

1. bpf_ns_per_packet is the primary overhead metric (from BPF runtime stats).
2. XDP is expected to be the cheapest (earliest path, no sk_buff).
3. TC sits in the middle (qdisc layer, sk_buff already allocated).
4. fentry is typically highest (full tracing overhead on receive path).
5. Compare *-perf-*.txt against baseline to quantify added cache misses / cycles.
