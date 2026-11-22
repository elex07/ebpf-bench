# ebpf-bench
Empirical Characterization of eBPF Tracing Overhead and Tail-Latency Jitter Under High-Frequency System Loads (ebpf-bench)


### Development Order
Step 1–2 (Harness & Noise Baseline): 
Lock down the OS/CPU isolation script and verify zero-load baseline variance 
(confirming p99.9 jitter is flat before attaching any eBPF programs).

Step 3–4 (Experiment Set A):
Benchmark Ring Buffer vs. Perf Event Array across varying event sizes (e.g., 64B vs. 512B) 
and burst rates, measuring event drops and consumer CPU overhead.

Step 5–6 (Experiment Set B): 
Benchmark XDP vs. TC vs. fentry/kprobe under high PPS (packets per second),
measuring p99/p99.9 latency and hardware cache misses (perf stat).

Step 7–8 (Paper & Artifact): Package the one-command runner 
and write the 4–6 page short paper following the IMRaD format.

### Progress
| Feedback | Status | What changed |
| --- | --- | --- |
| 1. Avoid Bash/Python loop overhead | Done | Created a proper C program (jitter_measure.c) using clock_gettime(CLOCK_MONOTONIC_RAW). The baseline script is now only a wrapper. |
| 2. Vendor-aware Turbo/Boost paths | Done | 01-isolate.sh now detects the scaling driver and uses the correct sysfs path for Intel (intel_pstate/no_turbo) vs AMD/generic (cpufreq/boost). |
| 3. IRQ affinity + RT priority | Done | Added clear IRQ guidance. Documented the recommended invocation: sudo chrt -f 99 taskset -c <core> ... |
| 4. Full machine state in every result | Done | Every baseline run now writes a detailed header (kernel, cmdline, governor, turbo/boost, SMT, CPU model, etc.). |

