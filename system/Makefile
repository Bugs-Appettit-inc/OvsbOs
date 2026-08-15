CC = gcc
CFLAGS = -ffreestanding -nostdlib -static -fno-PIC \
         -mno-red-zone -mno-mmx -mno-sse -mgeneral-regs-only \
         -nostartfiles -fno-asynchronous-unwind-tables \
         -fno-unwind-tables -Wl,--build-id=none \
         -fno-stack-protector -O1 -I include -I lib
LDFLAGS = -e _start -T libc/link.ld
OBJCOPY = objcopy
PACKER = python3 tools/macho_pack.py
ROOT_DIR := $(abspath ..)
BUILD = $(ROOT_DIR)/build/system
OBJ_DIR = $(BUILD)/obj

LIBC_SRC = libc/crt0.c libc/stdio.c libc/stdlib.c libc/string.c libc/tui.c
OWT_SRC = lib/owt/owt_base.c lib/owt/owt_draw.c lib/owt/owt_theme.c \
          lib/owt/owt_window.c lib/owt/owt_label.c lib/owt/owt_button.c \
          lib/owt/owt_textbox.c lib/owt/owt_statusbar.c lib/owt/owt_listview.c \
          lib/owt/owt_combobox.c lib/owt/owt_menu.c lib/owt/owt_dialog.c
WM_SRC = lib/wm/wm.c

PROGS = owt_test owt_app
PROGS_ELF = $(addprefix $(BUILD)/, $(addsuffix .elf, $(PROGS)))
PROGS_BIN = $(addprefix $(BUILD)/, $(addsuffix .bin, $(PROGS)))
PROGS_MACHO = $(addprefix $(BUILD)/, $(addsuffix .macho, $(PROGS)))

.PHONY: all clean

all: $(PROGS_MACHO)

$(BUILD) $(OBJ_DIR):
	mkdir -p $@

$(BUILD)/%.elf: progs/%.c $(LIBC_SRC) $(OWT_SRC) $(WM_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBC_SRC) $(OWT_SRC) $(WM_SRC)

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/%.macho: $(BUILD)/%.bin
	$(PACKER) $< $@

clean:
	find . -type f \( -name '*.o' -o -name '*.bin' -o -name '*.elf' -o -name '*.macho' \) -delete
	rm -rf $(BUILD)
	@echo "System limpo."
