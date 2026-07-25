# ♥ USERLAND MAKEFILE ~ "Compila os programinhas ring 3!"
CC = gcc
CFLAGS = -mno-sse -mno-sse2 -mno-sse3 -ffreestanding -nostdlib -static -fno-PIC \
         -mno-red-zone -mno-mmx -mno-sse -mgeneral-regs-only \
         -nostartfiles -fno-asynchronous-unwind-tables \
         -fno-unwind-tables -Wl,--build-id=none \
         -fno-stack-protector -O1 -I include
LDFLAGS = -e _start -T libc/link.ld
OBJCOPY = objcopy
PACKER = python3 tools/macho_pack.py
BUILD = ../build/userland
LIBC_SRC = libc/crt0.c libc/stdio.c libc/stdlib.c libc/string.c libc/tui.c

PROGS = desktop
PROGS_ELF = $(addprefix $(BUILD)/, $(addsuffix .elf, $(PROGS)))
PROGS_BIN = $(addprefix $(BUILD)/, $(addsuffix .bin, $(PROGS)))
PROGS_MACHO = $(addprefix $(BUILD)/, $(addsuffix .macho, $(PROGS)))

.PHONY: all clean

all: $(PROGS_MACHO)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.elf: progs/%.c $(LIBC_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBC_SRC)

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/%.macho: $(BUILD)/%.bin
	$(PACKER) $< $@

clean:
	rm -f $(PROGS_ELF) $(PROGS_BIN) $(PROGS_MACHO)
