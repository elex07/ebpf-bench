#!/usr/bin/env bash
# 01-isolate.sh
# Apply and verify CPU / OS isolation settings for low-noise benchmarking.
# Must be run as root.

set -euo pipefail

echo "=== eBPF Bench – Isolation Script ==="
echo "Timestamp: $(date -Iseconds)"
echo

if [[ $EUID -ne 0 ]]; then
    echo "ERROR: This script must be run as root (sudo)." >&2
    exit 1
fi

write_sysfs() {
    local path="$1"
    local value="$2"
    local desc="$3"
    if [[ -f "$path" ]]; then
        if echo "$value" > "$path" 2>/dev/null; then
            echo "    [OK] $desc  ($path = $value)"
            return 0
        else
            echo "    [FAIL] Could not write $value to $path"
            return 1
        fi
    else
        echo "    [SKIP] $path not present"
        return 1
    fi
}

echo "[1] Detecting CPU frequency scaling driver..."
SCALING_DRIVER="unknown"
if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver ]]; then
    SCALING_DRIVER=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)
    echo "    Detected: $SCALING_DRIVER"
else
    echo "    No cpufreq sysfs found (possible in some VMs)."
fi

echo "[2] Setting governor to 'performance'..."
if [[ -d /sys/devices/system/cpu/cpu0/cpufreq ]]; then
    for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        echo performance > "$gov" 2>/dev/null || true
    done
    CURRENT_GOV=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
    echo "    Current governor (cpu0): $CURRENT_GOV"
else
    echo "    cpufreq not available."
fi

echo "[3] Disabling Turbo Boost / Core Performance Boost..."
case "$SCALING_DRIVER" in
    intel_pstate|intel_cpufreq)
        write_sysfs "/sys/devices/system/cpu/intel_pstate/no_turbo" "1" "Intel no_turbo"
        ;;
    acpi-cpufreq|amd-pstate|amd-pstate-epp)
        write_sysfs "/sys/devices/system/cpu/cpufreq/boost" "0" "cpufreq boost"
        ;;
    *)
        write_sysfs "/sys/devices/system/cpu/intel_pstate/no_turbo" "1" "Intel no_turbo (fallback)" || true
        write_sysfs "/sys/devices/system/cpu/cpufreq/boost" "0" "Generic boost (fallback)" || true
        ;;
esac

echo "[4] SMT / Hyper-Threading status:"
if [[ -f /sys/devices/system/cpu/smt/control ]]; then
    echo "    Current: $(cat /sys/devices/system/cpu/smt/control)"
    echo "    (To disable permanently use kernel param: nosmt)"
else
    echo "    SMT control interface not present."
fi

echo "[5] IRQ Affinity guidance:"
echo "    Hardware interrupts can still preempt isolated cores."
echo "    Recommended: identify busy IRQs with 'cat /proc/interrupts'"
echo "    and move them away from measurement cores."
echo "    Strong isolation still requires boot parameters:"
echo "      isolcpus=<cpus> nohz_full=<cpus> rcu_nocbs=<cpus> nosmt"

echo
echo "=== Current Machine State ==="
echo "Kernel     : $(uname -r)"
echo "Cmdline    : $(cat /proc/cmdline)"
echo "CPU model  : $(grep -m1 'model name' /proc/cpuinfo || echo 'unknown')"
if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
    echo "Governor   : $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
fi
echo
echo "=== Isolation script finished ==="
