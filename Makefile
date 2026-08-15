# Makefile raiz do projeto OvsbOS

ROOT_DIR := $(CURDIR)
BUILD_DIR := $(ROOT_DIR)/build
KERNEL_DIR := $(ROOT_DIR)/kernel
SYSTEM_DIR := $(ROOT_DIR)/system

.PHONY: all kernel system run clean

all: run

kernel:
	$(MAKE) -C $(KERNEL_DIR) all

system:
	$(MAKE) -C $(SYSTEM_DIR) all

run:
	bash $(ROOT_DIR)/run.sh

clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	$(MAKE) -C $(SYSTEM_DIR) clean
	find $(ROOT_DIR) -type f \( -name '*.o' -o -name '*.bin' -o -name '*.elf' -o -name '*.macho' \) -delete
	@rm -rf $(BUILD_DIR)
	@echo "Projeto limpo."
