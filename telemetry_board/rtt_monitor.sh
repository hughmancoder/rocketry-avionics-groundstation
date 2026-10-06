#!/usr/bin/env bash
# Live console over the ST-Link (SEGGER RTT via OpenOCD) - no USB-UART needed.
# Streams everything the firmware printf()s, e.g. GPS fix/sats/lat/lon.
# Usage: ./rtt_monitor.sh   (Ctrl+C to stop; the board keeps running)
set -euo pipefail
cd "$(dirname "$0")"

PKG="$HOME/.platformio/packages"
OPENOCD="$PKG/tool-openocd"
ELF=".pio/build/stm32l433/firmware.elf"
PORT=19021

ADDR=$("$PKG/toolchain-gccarmnoneeabi/bin/arm-none-eabi-nm" "$ELF" | awk '$3=="_SEGGER_RTT"{print "0x"$1}')
[ -n "$ADDR" ] || { echo "_SEGGER_RTT not found in $ELF - build/flash first (make upload)"; exit 1; }

"$OPENOCD/bin/openocd" -s "$OPENOCD/openocd/scripts" -f board/st_nucleo_l4.cfg \
  -c "adapter speed 1800" -c "reset_config none" -c init \
  -c "rtt setup $ADDR 64 \"SEGGER RTT\"" -c "rtt start" -c "rtt server start $PORT 0" \
  >/tmp/rtt_openocd.log 2>&1 &
OCD_PID=$!
trap 'kill $OCD_PID 2>/dev/null' EXIT

for _ in $(seq 20); do nc -z localhost $PORT 2>/dev/null && break; sleep 0.25; done
echo "--- RTT console on localhost:$PORT (Ctrl+C to quit) ---"
nc -d localhost $PORT
