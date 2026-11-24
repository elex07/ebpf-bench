#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RESULTS_DIR="${EXP_DIR}/results"
WORKLOAD="${EXP_DIR}/workload/syscall_burst"

mkdir -p "${RESULTS_DIR}"

echo "=== Experiment Set A – RingBuf vs Perf comparison ==="

if [[ ! -x "${WORKLOAD}" ]]; then
	make -C "${EXP_DIR}" workload
fi

SIZES=(64 512)
RATES=(100000 500000)
CORES=(1 4)
DURATION=10
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
SUMMARY="${RESULTS_DIR}/summary-${TIMESTAMP}.csv"

echo "mechanism,size,rate,cores,duration,notes" > "${SUMMARY}"

for size in "${SIZES[@]}"; do
	for rate in "${RATES[@]}"; do
		for cores in "${CORES[@]}"; do
			echo "--- size=${size} rate=${rate} cores=${cores} ---"
			taskset -c 2-$((1+cores)) "${WORKLOAD}" \
				-c "${cores}" -s 2 -r "${rate}" -d "${DURATION}" -l \
				> "${RESULTS_DIR}/baseline-s${size}-r${rate}-c${cores}-${TIMESTAMP}.txt" 2>&1 || true
			echo "baseline,${size},${rate},${cores},${DURATION},no-bpf" >> "${SUMMARY}"
			echo "ringbuf,${size},${rate},${cores},${DURATION},placeholder" >> "${SUMMARY}"
			echo "perf,${size},${rate},${cores},${DURATION},placeholder" >> "${SUMMARY}"
		done
	done
done

echo "Summary written to ${SUMMARY}"
