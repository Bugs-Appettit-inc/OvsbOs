# Makefile raiz do OvsbOS
# Kernel: delega pro OvsbMk/ (Makefile interno).
# ISO: grub-mkrescue sobre OvsbMk/iso.

KERNEL_DIR := OvsbMk
ISO        := OvsbOS.iso

.PHONY: all kernel iso run run-curses run-test clean

# all: kernel + ISO
all: kernel iso

# kernel: compila o kernel
kernel:
	$(MAKE) -C $(KERNEL_DIR)

# iso: gera ISO bootável
iso: kernel
	cp $(KERNEL_DIR)/build/kernel.elf $(KERNEL_DIR)/iso/boot/kernel.elf
	grub-mkrescue -o $(ISO) $(KERNEL_DIR)/iso 2>/dev/null || true

# run: QEMU com VGA std (-rtc base=localtime acerta o relógio)
run: all
	qemu-system-x86_64 -enable-kvm -boot order=d -cdrom $(ISO) -m 512M \
		-serial stdio -rtc base=localtime \
		-drive file=disk.img,format=raw,index=0

# run-curses: QEMU em modo texto
run-curses: all
	qemu-system-x86_64 -enable-kvm -boot order=d -cdrom $(ISO) -m 512M \
		-display curses -rtc base=localtime \
		-drive file=disk.img,format=raw,index=0

# run-test: headless
run-test: all
	qemu-system-x86_64 -enable-kvm -boot order=d -cdrom $(ISO) -m 512M -no-reboot \
		-drive file=disk.img,format=raw,index=0 \
		-serial file:/tmp/ovsbos.log -display none -rtc base=localtime

# clean: limpa build
clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	rm -f $(ISO)
