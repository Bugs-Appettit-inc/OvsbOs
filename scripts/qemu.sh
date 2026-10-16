#!/bin/bash
# scripts/qemu.sh — consórdio dos 6 scripts de QEMU em 1 (Fase 5).
# Uso: qemu.sh [vga|gl|gl256|tcg|headless] [log]
#   vga      = SDL + virtio-vga      (padrao)
#   gl       = SDL + virtio-vga-gl   (VirGL 2D)
#   gl256    = gl com 256M ram
#   tcg      = sem KVM (debug)
#   headless = sem display (so log serial)
# Lista exit codes em /tmp/qemu-rc.log.
cd "$(dirname "$0")/.." || exit 1
MODE="${1:-vga}"
SER="${2:-/tmp/vga.log}"
BIN=/home/gabicnt/bin/qemu-system-x86_64
[ -x "$BIN" ] || BIN=qemu-system-x86_64
ACCEL=-enable-kvm; MEM=512M; DISPLAY="-device virtio-vga -display sdl"
case "$MODE" in
  gl)       DISPLAY="-device virtio-vga-gl -display sdl,gl=on";;
  gl256)    DISPLAY="-device virtio-vga-gl -display sdl,gl=on"; MEM=256M;;
  tcg)      ACCEL=""; DISPLAY="-device virtio-vga-gl -display sdl,gl=on";;
  headless) DISPLAY="";;
  *)  ;;
esac
echo "[$(date +%H:%M:%S)] qemu $MODE start" >> /tmp/qemu-rc.log
"$BIN" $ACCEL -boot order=d -cdrom OvsbOS.iso -m $MEM \
  -drive file=disk.img,format=raw,index=0,cache=none \
  $DISPLAY -serial file:"$SER" -rtc base=localtime -no-reboot \
  -monitor unix:/tmp/qmon,server,nowait "$@"
echo "QEMU_RC=$? mode=$MODE $(date +%H:%M:%S)" >> /tmp/qemu-rc.log
