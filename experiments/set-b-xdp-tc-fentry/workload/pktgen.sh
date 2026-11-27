#!/usr/bin/env bash
set -euo pipefail

VETH_TX=${VETH_TX:-veth_tx}
PKT_SIZE=${PKT_SIZE:-64}
PPS=${PPS:-100000}
DURATION=${DURATION:-10}
COUNT=$((PPS * DURATION))
DELAY_NS=$((1000000000 / PPS))
PGDEV=/proc/net/pktgen

if [[ ! -d "${PGDEV}" ]]; then
	echo "ERROR: pktgen not available. Run: sudo modprobe pktgen"
	exit 1
fi

echo "=== pktgen: count=${COUNT} size=${PKT_SIZE} pps=${PPS} delay_ns=${DELAY_NS} iface=${VETH_TX} ==="

echo "reset" > "${PGDEV}/pgctrl" 2>/dev/null || true
echo "rem_device_all" > "${PGDEV}/kpktgend_2" 2>/dev/null || true
echo "add_device ${VETH_TX}" > "${PGDEV}/kpktgend_2"

echo "count ${COUNT}"              > "${PGDEV}/${VETH_TX}"
echo "pkt_size ${PKT_SIZE}"        > "${PGDEV}/${VETH_TX}"
echo "delay ${DELAY_NS}"           > "${PGDEV}/${VETH_TX}"   # critical for PPS pacing
echo "clone_skb 1000"              > "${PGDEV}/${VETH_TX}"   # reduce TX alloc overhead
echo "dst_mac 02:00:00:00:00:02"   > "${PGDEV}/${VETH_TX}"   # matches setup_veth.sh
echo "dst 10.253.1.2"              > "${PGDEV}/${VETH_TX}"
echo "flag QUEUE_MAP_CPU"          > "${PGDEV}/${VETH_TX}" 2>/dev/null || true

# Blocks until count packets are sent (no extra sleep needed)
echo "start" > "${PGDEV}/pgctrl"

echo "pktgen finished"
cat "${PGDEV}/${VETH_TX}" | head -25
