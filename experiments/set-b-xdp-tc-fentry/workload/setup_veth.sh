#!/usr/bin/env bash
set -euo pipefail

VETH_TX=${VETH_TX:-veth_tx}
VETH_RX=${VETH_RX:-veth_rx}

echo "=== Setting up veth pair: ${VETH_TX} <-> ${VETH_RX} ==="

ip link del "${VETH_TX}" 2>/dev/null || true
ip link del "${VETH_RX}" 2>/dev/null || true

ip link add "${VETH_TX}" type veth peer name "${VETH_RX}"

# Deterministic MAC so pktgen dst_mac matches
ip link set dev "${VETH_RX}" address 02:00:00:00:00:02
ip link set dev "${VETH_TX}" address 02:00:00:00:00:01

ip link set "${VETH_TX}" up
ip link set "${VETH_RX}" up

ip addr add 10.253.1.1/24 dev "${VETH_TX}" 2>/dev/null || true
ip addr add 10.253.1.2/24 dev "${VETH_RX}" 2>/dev/null || true

echo "veth pair ready:"
ip -br link show "${VETH_TX}"
ip -br link show "${VETH_RX}"
echo "RX MAC: $(cat /sys/class/net/${VETH_RX}/address)"
echo "Attach BPF programs to: ${VETH_RX}"
echo "Generate traffic from:  ${VETH_TX}"
