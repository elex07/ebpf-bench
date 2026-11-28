# Experiment Set B — XDP vs TC vs fentry/kprobe

## Goal

Compare overhead of three eBPF attachment points on the **ingress (RX)** path under controlled high packet rate:

- **XDP** (driver level, earliest)
- **TC** (qdisc / traffic control)
- **fentry** on `__netif_receive_skb_core` (tracing)

## Methodology Highlights

- Isolated **veth pair** (`veth_tx` → `veth_rx`)
- Kernel **pktgen** with nanosecond `delay` pacing + `clone_skb`
- Deterministic MAC alignment
- BPF runtime stats → `bpf_ns_per_packet`
- Optional `perf stat -C 2` for cycles / cache-misses
- Same ifindex filter on all three programs

## How to Run

```bash
cd experiments/set-b-xdp-tc-fentry
bpftool btf dump file /sys/kernel/btf/vmlinux format c > bpf/vmlinux.h
make all
sudo ./scripts/run_comparison.sh
```

---

| Metric | Meaning |
| --- | --- |
| bpf_ns_per_packet | Average BPF program execution time |
| packets_seen / packets_dropped | Correctness under load |
| perf stat counters | Hardware cost (cycles, cache-misses) |

---

