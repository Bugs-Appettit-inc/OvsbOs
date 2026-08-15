CC := gcc
NASM := nasm
CFLAGS := -ffreestanding -nostdlib -mno-red-zone -mno-mmx -mno-sse
CFLAGS += -mgeneral-regs-only -m64 -O2 -Wall
LDFLAGS := -nostdlib -no-pie -Wl,-nostdlib -T linker.ld

ROOT_DIR := $(abspath ..)
BUILD_DIR := $(ROOT_DIR)/build/kernel
OBJ_DIR := $(BUILD_DIR)/obj
KERNEL_ELF := $(BUILD_DIR)/kernel.elf

KERNEL_DIRS := . drivers fs gui wm
VPATH := $(KERNEL_DIRS)

C_SRCS := $(notdir $(wildcard *.c drivers/*.c fs/*.c gui/*.c wm/*.c))
ASM_SRCS := boot64.o idt_asm.o keyboard_asm.o switch_asm.o
OBJS := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o) $(ASM_SRCS))
INC := $(addprefix -I,$(KERNEL_DIRS))

.PHONY: all clean

all: $(KERNEL_ELF)

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/boot64.o: boot64.asm | $(OBJ_DIR)
	$(NASM) -f elf64 -o $@ $<

$(OBJ_DIR)/idt_asm.o: idt.asm | $(OBJ_DIR)
	$(NASM) -f elf64 -o $@ $<

$(OBJ_DIR)/keyboard_asm.o: drivers/keyboard_asm.asm | $(OBJ_DIR)
	$(NASM) -f elf64 -o $@ $<

$(OBJ_DIR)/switch_asm.o: switch.asm | $(OBJ_DIR)
	$(NASM) -f elf64 -o $@ $<

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(KERNEL_ELF): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	find . -type f \( -name '*.o' -o -name '*.bin' -o -name '*.elf' -o -name '*.macho' \) -delete
	rm -rf $(BUILD_DIR)
	@echo "Kernel limpo."
