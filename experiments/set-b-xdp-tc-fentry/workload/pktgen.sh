#!/usr/bin/env bash
set -euo pipefail

VETH_TX=${VETH_TX:-veth_tx}
PKT_SIZE=${PKT_SIZE:-64}
PPS=${PPS:-100000}
DURATION=${DURATION:-10}
COUNT=$((PPS * DURATION))
PGDEV=/proc/net/pktgen

if [[ ! -d "${PGDEV}" ]]; then
	echo "ERROR: pktgen not available. Run: sudo modprobe pktgen"
	exit 1
fi

echo "=== pktgen: ${COUNT} packets, size=${PKT_SIZE}, iface=${VETH_TX} ==="

echo "reset" > "${PGDEV}/pgctrl" 2>/dev/null || true
echo "rem_device_all" > "${PGDEV}/kpktgend_2" 2>/dev/null || true
echo "add_device ${VETH_TX}" > "${PGDEV}/kpktgend_2"

echo "count ${COUNT}"            > "${PGDEV}/${VETH_TX}"
echo "pkt_size ${PKT_SIZE}"      > "${PGDEV}/${VETH_TX}"
echo "dst_mac 02:00:00:00:00:02" > "${PGDEV}/${VETH_TX}"
echo "dst 10.253.1.2"            > "${PGDEV}/${VETH_TX}"
echo "flag QUEUE_MAP_CPU"        > "${PGDEV}/${VETH_TX}" 2>/dev/null || true

echo "start" > "${PGDEV}/pgctrl"
sleep $((DURATION + 2))

echo "pktgen finished"
cat "${PGDEV}/${VETH_TX}" | head -20
