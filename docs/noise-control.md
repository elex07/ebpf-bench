# Noise Control & Baseline (Steps 1–2)

Before any eBPF experiment, we must reduce measurement noise as much as possible and quantify the remaining jitter.

## Goals

- Minimize frequency scaling, Turbo Boost, SMT, and IRQ noise
- Establish a zero-load baseline
- Record p99.9 and p99.99 timer jitter
- Only proceed when the baseline is stable

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/01-isolate.sh` | Apply and report isolation settings |
| `scripts/02-baseline-jitter.sh` | Measure zero-load jitter using a C binary |
| `scripts/jitter_measure.c` | Actual measurement tool (`CLOCK_MONOTONIC_RAW` + `clock_nanosleep`) |

## Recommended Command Sequence

```bash
gcc -O2 -o scripts/jitter_measure scripts/jitter_measure.c

sudo ./scripts/01-isolate.sh

sudo chrt -f 99 taskset -c 2 ./scripts/02-baseline-jitter.sh 30
```

## What Good Isolation Looks Like

| Metric | Good | Acceptable | Problematic |
|--------|------|------------|-------------|
| p99.9  | < 200 µs | 200–500 µs | > 1 ms |
| p99.99 | < 500 µs | 500 µs – 2 ms | Multi-ms spikes |
| Run-to-run variation | Small | Moderate | Large |

## Sample Results

Ready-made sample outputs are available in the `baseline/` directory so you do not need to run the scripts to understand the expected results.

## Strong Isolation (Boot Parameters)

For best results, add to the kernel command line:

```text
isolcpus=2-5 nohz_full=2-5 rcu_nocbs=2-5 nosmt
```

Then reboot.
