#!/bin/bash
cd "$(dirname "$0")/.." || exit 1
exec qemu-system-x86_64 -accel tcg -boot order=d -cdrom OvsbOS.iso -m 512M \
  -drive file=disk.img,format=raw,index=0,cache=none -device virtio-vga-gl \
  -display sdl,gl=on -serial file:/tmp/vga.log -rtc base=localtime -no-reboot \
  -monitor unix:/tmp/qmon,server,nowait
