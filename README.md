# eBPF Bench

**Quantitative comparison of eBPF data transfer mechanisms under controlled load** : Empirical Characterization of eBPF Tracing Overhead and Tail-Latency Jitter Under High-Frequency System Loads (ebpf-bench)

This repository contains a rigorous benchmarking harness for comparing `BPF_MAP_TYPE_RINGBUF` and `BPF_MAP_TYPE_PERF_EVENT_ARRAY`, with strong focus on noise control, producer-side overhead, and reproducibility.

---

### Development Order
#### Step 1–2 (Harness & Noise Baseline):  
Lock down the OS/CPU isolation script and verify zero-load baseline variance (confirming p99.9 jitter is flat before attaching any eBPF programs).

#### Step 3–4 (Experiment Set A):  
Benchmark Ring Buffer vs. Perf Event Array across varying event sizes (e.g., 64B vs. 512B) and burst rates, measuring event drops and consumer CPU overhead.

#### Step 5–6 (Experiment Set B):   
Benchmark XDP vs. TC vs. fentry/kprobe under high PPS (packets per second), measuring p99/p99.9 latency and hardware cache misses (perf stat).

#### Step 7–8 (Paper & Artifact):  
Package the one-command runner and write the 4–6 page short paper following the IMRaD format.

---

### Repository Structure

```text
ebpf-bench/                          # Top-level repository root
├── README.md
├── LICENSE
├── docs/
│   ├── development.md
│   ├── experiment-set-a.md
│   ├── experiment-set-b.md
│   ├── noise-control.md
│   ├── overview.md
│   ├── paper-outline-imrad.md
│   └── results-interpretation.md
├── scripts/                         # Global helper scripts (isolation, baseline)
│   ├── 01-isolate.sh
│   ├── 02-baseline-jitter.sh
│   ├── jitter_measure.c
│   └── jitter_measure
├── baseline/                        # Results from zero-load baseline runs
├── experiments/
│   ├── set-a-ringbuf-vs-perf       # Experiment Set A (current focus)
│   │   ├── README.md
│   │   ├── Makefile
│   │   ├── bpf/
│   │   │   ├── common.h
│   │   │   ├── ringbuf_producer.bpf.c
│   │   │   └── perf_producer.bpf.c
│   │   ├── user/
│   │   │   ├── ringbuf_consumer.c
│   │   │   └── perf_consumer.c
│   │   ├── workload/
│   │   │   └── syscall_burst.c
│   │   ├── scripts/
│   │   │   └── run_comparison.sh
│   │   └── results/
│   └── set-b-xdp-tc-fentry
│       ├── README.md
│       ├── Makefile
│       ├── bpf
│       │   ├── common.h
│       │   ├── fentry_prog.bpf.c
│       │   ├── tc_prog.bpf.c
│       │   └── xdp_prog.bpf.c       │   
│       ├── user
│       │   ├── fentry_loader.c
│       │   ├── tc_loader.c
│       │   └── xdp_loader.c
│       ├── workload
│       │   ├── pktgen.sh
│       │   └── setup_veth.sh
│       ├── scripts
│       │   └── run_comparison.sh
│       └── results/  
└── results/                         # Optional top-level aggregated results

```
---

### Progress
| Feedback | Status | What changed |
| --- | --- | --- |
| 1. Avoid Bash/Python loop overhead | Done | Created a proper C program (jitter_measure.c) using clock_gettime(CLOCK_MONOTONIC_RAW). The baseline script is now only a wrapper. |
| 2. Vendor-aware Turbo/Boost paths | Done | 01-isolate.sh now detects the scaling driver and uses the correct sysfs path for Intel (intel_pstate/no_turbo) vs AMD/generic (cpufreq/boost). |
| 3. IRQ affinity + RT priority | Done | Added clear IRQ guidance. Documented the recommended invocation: sudo chrt -f 99 taskset -c <core> ... |
| 4. Full machine state in every result | Done | Every baseline run now writes a detailed header (kernel, cmdline, governor, turbo/boost, SMT, CPU model, etc.). |

---

### Quick Start (Baseline)

#### 1. Compile the measurement tool
gcc -O2 -o scripts/jitter_measure scripts/jitter_measure.c

#### 2. Apply isolation settings
sudo ./scripts/01-isolate.sh

#### 3. Run zero-load baseline (recommended way)
sudo chrt -f 99 taskset -c 2 ./scripts/02-baseline-jitter.sh 30

---

> Pavitra Kanetkar

