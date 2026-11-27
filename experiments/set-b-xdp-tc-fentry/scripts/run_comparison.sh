#!/usr/bin/env bash
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
	echo "ERROR: Run as root (sudo)." >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RESULTS_DIR="${EXP_DIR}/results"
VETH_RX=${VETH_RX:-veth_rx}
VETH_TX=${VETH_TX:-veth_tx}

mkdir -p "${RESULTS_DIR}"

echo "=== Experiment Set B – XDP vs TC vs fentry ==="

if [[ ! -x "${EXP_DIR}/user/xdp_loader" ]]; then
	echo "Building..."
	make -C "${EXP_DIR}" all || {
		echo "Build failed. Need clang, bpftool, libbpf-dev, and bpf/vmlinux.h"
		exit 1
	}
fi

echo "Setting up veth pair..."
"${EXP_DIR}/workload/setup_veth.sh"
modprobe pktgen 2>/dev/null || true

SIZES=(64 1500)
PPS_LIST=(100000 500000)
DURATION=10
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
SUMMARY="${RESULTS_DIR}/summary-${TIMESTAMP}.csv"

echo "mechanism,pkt_size,pps,duration,packets_seen,packets_dropped,bpf_ns_per_packet,notes" > "${SUMMARY}"

run_one() {
	local mech="$1"
	local loader="$2"
	local size="$3"
	local pps="$4"
	local outfile="${RESULTS_DIR}/${mech}-s${size}-p${pps}-${TIMESTAMP}.txt"
	local perf_out="${RESULTS_DIR}/${mech}-s${size}-p${pps}-perf-${TIMESTAMP}.txt"

	echo "--- ${mech} size=${size} pps=${pps} ---"

	"${loader}" -i "${VETH_RX}" -d $((DURATION + 5)) > "${outfile}" 2>&1 &
	local loader_pid=$!
	sleep 0.5

	if ! kill -0 ${loader_pid} 2>/dev/null; then
		echo "  Loader failed. Check ${outfile}"
		echo "${mech},${size},${pps},${DURATION},0,0,0,loader-failed" >> "${SUMMARY}"
		return
	fi

	if command -v perf >/dev/null 2>&1; then
		PKT_SIZE=${size} PPS=${pps} DURATION=${DURATION} \
		perf stat -C 2 \
			-e cycles,instructions,cache-misses,L1-dcache-load-misses,LLC-load-misses \
			-o "${perf_out}" \
			"${EXP_DIR}/workload/pktgen.sh" >> "${outfile}" 2>&1 || true
	else
		PKT_SIZE=${size} PPS=${pps} DURATION=${DURATION} \
			"${EXP_DIR}/workload/pktgen.sh" >> "${outfile}" 2>&1 || true
	fi

	kill -INT ${loader_pid} 2>/dev/null || true
	wait ${loader_pid} 2>/dev/null || true

	local seen dropped nspkt
	seen=$(grep -oP 'packets_seen\s*:\s*\K[0-9]+' "${outfile}" | tail -1 || echo 0)
	dropped=$(grep -oP 'packets_dropped\s*:\s*\K[0-9]+' "${outfile}" | tail -1 || echo 0)
	nspkt=$(grep -oP 'bpf_ns_per_packet\s*:\s*\K[0-9.]+' "${outfile}" | tail -1 || echo 0)

	echo "${mech},${size},${pps},${DURATION},${seen},${dropped},${nspkt},ok" >> "${SUMMARY}"
	echo "  done → ${outfile}"
}

for size in "${SIZES[@]}"; do
	for pps in "${PPS_LIST[@]}"; do
		echo "--- baseline size=${size} pps=${pps} ---"
		outfile="${RESULTS_DIR}/baseline-s${size}-p${pps}-${TIMESTAMP}.txt"
		PKT_SIZE=${size} PPS=${pps} DURATION=${DURATION} \
			"${EXP_DIR}/workload/pktgen.sh" > "${outfile}" 2>&1 || true
		echo "baseline,${size},${pps},${DURATION},0,0,0,no-bpf" >> "${SUMMARY}"
	done
done

for size in "${SIZES[@]}"; do
	for pps in "${PPS_LIST[@]}"; do
		run_one "xdp"    "${EXP_DIR}/user/xdp_loader"    "${size}" "${pps}"
		run_one "tc"     "${EXP_DIR}/user/tc_loader"     "${size}" "${pps}"
		run_one "fentry" "${EXP_DIR}/user/fentry_loader" "${size}" "${pps}"
	done
done

ip link del "${VETH_TX}" 2>/dev/null || true

echo
echo "Summary written to ${SUMMARY}"
echo "All runs completed."
