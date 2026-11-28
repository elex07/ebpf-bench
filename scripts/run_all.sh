#!/usr/bin/env bash
# run_all.sh – One-command reproduction of the full eBPF Bench suite
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
	echo "ERROR: Run as root (sudo ./scripts/run_all.sh)" >&2
	exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

echo "=============================================="
echo " eBPF Bench – Full Reproduction"
echo "=============================================="
echo

if [[ "${SKIP_BASELINE:-0}" != "1" ]]; then
	echo "[1/3] Isolation + zero-load baseline"
	if [[ -x scripts/01-isolate.sh ]]; then
		./scripts/01-isolate.sh || true
	fi
	if [[ -x scripts/02-baseline-jitter.sh ]]; then
		chrt -f 99 taskset -c 2 ./scripts/02-baseline-jitter.sh 20 || true
	fi
	echo
fi

echo "[2/3] Experiment Set A – RingBuf vs Perf"
if [[ -d experiments/set-a-ringbuf-vs-perf ]]; then
	pushd experiments/set-a-ringbuf-vs-perf >/dev/null
	if [[ ! -f bpf/vmlinux.h ]]; then
		bpftool btf dump file /sys/kernel/btf/vmlinux format c > bpf/vmlinux.h
	fi
	make all || true
	./scripts/run_comparison.sh || true
	popd >/dev/null
fi
echo

echo "[3/3] Experiment Set B – XDP vs TC vs fentry"
if [[ -d experiments/set-b-xdp-tc-fentry ]]; then
	pushd experiments/set-b-xdp-tc-fentry >/dev/null
	if [[ ! -f bpf/vmlinux.h ]]; then
		bpftool btf dump file /sys/kernel/btf/vmlinux format c > bpf/vmlinux.h
	fi
	make all || true
	./scripts/run_comparison.sh || true
	popd >/dev/null
fi

echo
echo "=============================================="
echo " Done. Results are under:"
echo "   experiments/set-a-ringbuf-vs-perf/results/"
echo "   experiments/set-b-xdp-tc-fentry/results/"
echo "=============================================="
