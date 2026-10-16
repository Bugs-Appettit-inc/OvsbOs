#!/bin/bash
# QEMU SDL VGA padrão (sem GL)
cd "$(dirname "$0")/.." || exit 1
exec qemu-system-x86_64 -enable-kvm -boot order=d -cdrom OvsbOS.iso -m 512M \
  -drive file=disk.img,format=raw,index=0 \
  -serial file:/tmp/vga.log -rtc base=localtime -no-reboot \
  -display sdl -monitor unix:/tmp/qmon,server,nowait
