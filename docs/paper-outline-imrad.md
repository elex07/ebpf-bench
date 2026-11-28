
---

### 3. `docs/paper-outline-imrad.md`

```markdown
# IMRaD Short Paper Outline

## Working Title

**“Measuring the Cost of Observability: A Controlled Comparison of eBPF Data Paths and Attachment Points”**

Target venues: eBPF Workshop (SIGCOMM), Linux Plumbers Conference, ACM/SPEC ICPE (short paper / poster track).

---

## 1. Introduction

- Observability and tracing are increasingly implemented with eBPF.
- Two practical questions:
  1. Which data-transfer mechanism (Ring Buffer vs Perf Event Array)?
  2. Where to attach (XDP vs TC vs fentry/kprobe) if overhead matters?
- Existing comparisons often miss producer-side cost, equal memory, or noise control.
- **Contribution**: Reproducible, noise-controlled benchmark suite and quantitative results for both questions.

## 2. Methods

### 2.1 Experimental Platform
- Kernel version, CPU model, isolation (`isolcpus`, performance governor, Turbo off).
- Zero-load baseline (timer jitter p99.9 / p99.99).

### 2.2 Experiment Set A – Data Transfer Mechanisms
- Workload: controlled multi-core `getpid` burst generator.
- Mechanisms: Ring Buffer vs Perf Event Array.
- Equal total buffer memory (4 MiB, power-of-two pages for Perf).
- Metrics: producer p99/p99.9, drops, out-of-order, consumer CPU.

### 2.3 Experiment Set B – Attachment Points
- Workload: paced pktgen over isolated veth pair (ingress).
- Mechanisms: XDP, TC ingress, fentry on `__netif_receive_skb_core`.
- Metrics: `bpf_ns_per_packet`, packets seen/dropped, `perf stat`.

### 2.4 Fairness & Reproducibility
- One-command runner (`scripts/run_all.sh`).
- Full scripts, BPF programs, and sample results in the artifact.

## 3. Results

### 3.1 Set A (replace with your numbers)
- RingBuf: lower drops, zero out-of-order under multi-core load.
- Perf: per-CPU reordering and higher drops at high rates.
- Producer p99.9 latency increases when either mechanism is attached.

### 3.2 Set B (replace with your numbers)
- XDP ≈ 38 ns/pkt, TC ≈ 90 ns/pkt, fentry ≈ 145 ns/pkt (64 B @ 100 kpps).
- Matching increase in cache-misses and cycles vs baseline.

## 4. Discussion

- Why the ordering holds (xdp_buff vs sk_buff, early vs late path, tracing cost).
- When higher-cost hooks are still the right choice.
- Limitations: single-machine veth, synthetic traffic, kernel-specific symbols.
- Threats to validity and mitigations (isolation, equal memory, paced load).

## 5. Related Work

- Prior eBPF / XDP performance studies and tracing-overhead papers.
- Position this work as dual-axis (data path + attachment) with a full artifact.

## 6. Conclusion

- Quantified evidence for two common design choices.
- Artifact allows others to reproduce and extend the measurements.
- Future work: more hooks, multi-host traffic, production-tracer integration.

---

## Artifact Checklist

- [ ] `scripts/run_all.sh` reproduces both experiment sets
- [ ] Sample results under `experiments/*/results/`
- [ ] Docs explain methodology and result interpretation
- [ ] Dependencies listed (clang, bpftool, libbpf, pktgen, perf)
