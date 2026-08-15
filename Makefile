# Makefile raiz do projeto OvsbOS

ROOT_DIR := $(CURDIR)
BUILD_DIR := $(ROOT_DIR)/build
KERNEL_DIR := $(ROOT_DIR)/kernel
SYSTEM_DIR := $(ROOT_DIR)/system

.PHONY: all kernel system run clean

all: kernel system

kernel:
	$(MAKE) -C $(KERNEL_DIR) all

system:
	$(MAKE) -C $(SYSTEM_DIR) all

run:
	@echo "Uso do dev script recomendado: python3 $(ROOT_DIR)/dev.py run"
	@echo "Ou: python3 $(ROOT_DIR)/dev.py all"
	@echo "Build direto: make kernel && make system"

clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	$(MAKE) -C $(SYSTEM_DIR) clean
	find $(ROOT_DIR) -type f \( -name '*.o' -o -name '*.bin' -o -name '*.elf' -o -name '*.macho' \) -delete
	@rm -rf $(BUILD_DIR)
	@echo "Projeto limpo."
