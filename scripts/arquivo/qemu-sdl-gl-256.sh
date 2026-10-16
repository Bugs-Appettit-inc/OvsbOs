#!/bin/bash
exec qemu-system-x86_64 -enable-kvm -boot order=d -cdrom OvsbOS.iso -m 256M -drive file=disk.img,format=raw,index=0 -device virtio-vga-gl -display sdl,gl=on -serial file:/tmp/vga.log -rtc base=localtime -no-reboot -monitor unix:/tmp/qmon,server,nowait
