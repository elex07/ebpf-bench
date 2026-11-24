# Development Guide

## Building the Baseline Tools

```bash
gcc -O2 -o scripts/jitter_measure scripts/jitter_measure.c
chmod +x scripts/*.sh
```

## Building Experiment Set A Workload

```bash
cd experiments/set-a-ringbuf-vs-perf/workload
gcc -O2 -Wall -o syscall_burst syscall_burst.c -lpthread
```

## Adding a New Experiment

1. Create a new directory under `experiments/`
2. Follow the same layout (`bpf/`, `user/`, `workload/`, `scripts/`, `results/`)
3. Document it in `docs/`
4. Keep total memory footprint equal when comparing mechanisms
5. Always measure producer-side latency

## Coding Conventions

- Use `CLOCK_MONOTONIC_RAW` for latency measurements
- Prefer explicit CPU pinning (`taskset` or `pthread_setaffinity_np`)
- Record full machine state with every important run
- Keep sample results in the repository so others do not need to re-run everything

## Knowledge Transfer

New contributors should start with:

1. Parent `README.md`
2. `docs/overview.md`
3. `docs/noise-control.md`
4. `baseline/` sample outputs
