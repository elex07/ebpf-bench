# Experiment Set B: XDP vs TC vs fentry/kprobe

## Goal

Compare the overhead and behaviour of three different eBPF attachment points under high packet rate:

- **XDP** (earliest, driver-level)
- **TC** (Traffic Control / qdisc layer)
- **fentry / kprobe** (kernel function tracing)

## Locked Design Principles (carried from Set A)

1. Controlled, reproducible workload
2. Fair comparison (same measurement methodology)
3. Producer-side + consumer-side metrics
4. Strong noise control (CPU pinning, isolation)
5. Clear drop / error accounting

## Directory Layout

```text
set-b-xdp-tc-fentry/
├── README.md
├── Makefile
├── bpf/
│   ├── common.h
│   ├── xdp_prog.bpf.c
│   ├── tc_prog.bpf.c
│   └── fentry_prog.bpf.c
├── user/
│   ├── xdp_loader.c
│   ├── tc_loader.c
│   └── fentry_loader.c
├── workload/
│   └── pktgen.sh
├── scripts/
│   └── run_comparison.sh
└── results/

```


