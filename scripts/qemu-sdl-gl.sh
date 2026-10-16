#!/bin/bash
# QEMU SDL + virtio-vga-gl (estável aqui; GTK morre sozinho)
cd "$(dirname "$0")/.." || exit 1
exec qemu-system-x86_64 -enable-kvm -boot order=d -cdrom OvsbOS.iso -m 512M \
  -drive file=disk.img,format=raw,index=0 -device virtio-vga-gl \
  -display sdl,gl=on -serial file:/tmp/vga.log -rtc base=localtime -no-reboot \
  -monitor unix:/tmp/qmon,server,nowait
