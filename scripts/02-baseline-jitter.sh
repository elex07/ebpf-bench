#!/usr/bin/env bash
# 02-baseline-jitter.sh
# Zero-load baseline jitter measurement using a compiled C binary
# (CLOCK_MONOTONIC_RAW) for accurate nanosecond measurements.
#
# Recommended invocation:
#   sudo chrt -f 99 taskset -c 2 ./scripts/02-baseline-jitter.sh [duration_sec]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
MEASURE_BIN="${SCRIPT_DIR}/jitter_measure"
OUTPUT_DIR="${ROOT_DIR}/baseline"
DURATION=${1:-20}

TIMESTAMP=$(date +%Y%m%d-%H%M%S)
RESULT_FILE="${OUTPUT_DIR}/jitter-${TIMESTAMP}.txt"

mkdir -p "${OUTPUT_DIR}"

echo "=== eBPF Bench – Zero-Load Baseline Jitter ==="
echo "Timestamp : $(date -Iseconds)"
echo "Duration  : ${DURATION} seconds"
echo "Binary    : ${MEASURE_BIN}"
echo

if [[ ! -x "${MEASURE_BIN}" ]]; then
    echo "ERROR: ${MEASURE_BIN} not found or not executable."
    echo "Compile it with:"
    echo "  gcc -O2 -o scripts/jitter_measure scripts/jitter_measure.c"
    exit 1
fi

# Save full machine state
{
    echo "=== Machine State ==="
    echo "Date          : $(date -Iseconds)"
    echo "Hostname      : $(hostname)"
    echo "Kernel        : $(uname -r)"
    echo "CPU model     : $(grep -m1 'model name' /proc/cpuinfo || echo unknown)"
    echo "Cmdline       : $(cat /proc/cmdline)"
    echo
    if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver ]]; then
        echo "Scaling driver: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)"
    fi
    if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
        echo "Governor      : $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
    fi
    if [[ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then
        echo "Intel no_turbo: $(cat /sys/devices/system/cpu/intel_pstate/no_turbo)"
    fi
    if [[ -f /sys/devices/system/cpu/cpufreq/boost ]]; then
        echo "cpufreq boost : $(cat /sys/devices/system/cpu/cpufreq/boost)"
    fi
    if [[ -f /sys/devices/system/cpu/smt/control ]]; then
        echo "SMT control   : $(cat /sys/devices/system/cpu/smt/control)"
    fi
    echo
    echo "=== Measurement (CLOCK_MONOTONIC_RAW) ==="
} | tee "${RESULT_FILE}"

echo "Running jitter_measure..."
echo "Best practice: sudo chrt -f 99 taskset -c <core> $0"
echo

"${MEASURE_BIN}" "${DURATION}" | tee -a "${RESULT_FILE}"

echo
echo "Results written to: ${RESULT_FILE}"
echo
echo "Focus on p99.9_ns and p99.99_ns. Run multiple times to check stability."
