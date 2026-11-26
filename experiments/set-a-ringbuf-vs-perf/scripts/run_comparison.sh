#!/usr/bin/env bash
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
	echo "ERROR: This script must be run as root (sudo)." >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RESULTS_DIR="${EXP_DIR}/results"
WORKLOAD="${EXP_DIR}/workload/syscall_burst"
RINGBUF_CONS="${EXP_DIR}/user/ringbuf_consumer"
PERF_CONS="${EXP_DIR}/user/perf_consumer"

mkdir -p "${RESULTS_DIR}"

echo "=== Experiment Set A – RingBuf vs Perf comparison ==="
echo "Results: ${RESULTS_DIR}"
echo

if [[ ! -x "${WORKLOAD}" || ! -x "${RINGBUF_CONS}" || ! -x "${PERF_CONS}" ]]; then
	echo "Building..."
	make -C "${EXP_DIR}" all || {
		echo "Build failed. Make sure clang, bpftool, libbpf-dev are installed."
		exit 1
	}
fi

SIZES=(64 512)
RATES=(100000 500000)
CORES=(1 4)
DURATION=10
CONSUMER_CORE=6
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
SUMMARY="${RESULTS_DIR}/summary-${TIMESTAMP}.csv"

echo "mechanism,size,rate,cores,duration,events_received,total_dropped,out_of_order,consumer_cpu_s,notes" > "${SUMMARY}"

run_one() {
	local mech="$1"
	local size="$2"
	local rate="$3"
	local cores="$4"
	local cons_bin="$5"
	local outfile="${RESULTS_DIR}/${mech}-s${size}-r${rate}-c${cores}-${TIMESTAMP}.txt"

	echo "--- ${mech} size=${size} rate=${rate} cores=${cores} ---"

	taskset -c ${CONSUMER_CORE} "${cons_bin}" -s "${size}" -d $((DURATION + 3)) \
		> "${outfile}" 2>&1 &
	local cons_pid=$!
	sleep 0.4

	if ! kill -0 ${cons_pid} 2>/dev/null; then
		echo "  ERROR: consumer failed to start / attach. Check ${outfile}"
		echo "${mech},${size},${rate},${cores},${DURATION},0,0,0,0,attach-failed" >> "${SUMMARY}"
		return
	fi

	local start_cpu=2
	taskset -c ${start_cpu}-$((start_cpu + cores - 1)) \
		"${WORKLOAD}" -c "${cores}" -s ${start_cpu} -r "${rate}" -d "${DURATION}" -l \
		>> "${outfile}" 2>&1 || true

	kill -INT ${cons_pid} 2>/dev/null || true
	wait ${cons_pid} 2>/dev/null || true

	local received dropped ooo cpu
	received=$(grep -oP 'events_received\s*:\s*\K[0-9]+' "${outfile}" | tail -1 || echo 0)
	dropped=$(grep -oP 'total_dropped\s*:\s*\K[0-9]+' "${outfile}" | tail -1 || echo 0)
	ooo=$(grep -oP 'out_of_order_events\s*:\s*\K[0-9]+' "${outfile}" | tail -1 || echo 0)
	cpu=$(grep -oP 'consumer_total_sec\s*:\s*\K[0-9.]+' "${outfile}" | tail -1 || echo 0)

	echo "${mech},${size},${rate},${cores},${DURATION},${received},${dropped},${ooo},${cpu},ok" >> "${SUMMARY}"
	echo "  done → ${outfile}"
}

# Baseline (no BPF)
for size in "${SIZES[@]}"; do
	for rate in "${RATES[@]}"; do
		for cores in "${CORES[@]}"; do
			echo "--- baseline size=${size} rate=${rate} cores=${cores} ---"
			outfile="${RESULTS_DIR}/baseline-s${size}-r${rate}-c${cores}-${TIMESTAMP}.txt"
			taskset -c 2-$((1+cores)) "${WORKLOAD}" \
				-c "${cores}" -s 2 -r "${rate}" -d "${DURATION}" -l \
				> "${outfile}" 2>&1 || true
			echo "baseline,${size},${rate},${cores},${DURATION},0,0,0,0,no-bpf" >> "${SUMMARY}"
		done
	done
done

# RingBuf runs
for size in "${SIZES[@]}"; do
	for rate in "${RATES[@]}"; do
		for cores in "${CORES[@]}"; do
			run_one "ringbuf" "${size}" "${rate}" "${cores}" "${RINGBUF_CONS}"
		done
	done
done

# Perf runs
for size in "${SIZES[@]}"; do
	for rate in "${RATES[@]}"; do
		for cores in "${CORES[@]}"; do
			run_one "perf" "${size}" "${rate}" "${cores}" "${PERF_CONS}"
		done
	done
done

echo
echo "Summary written to ${SUMMARY}"
echo "All runs completed."
