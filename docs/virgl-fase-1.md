# FASE 1 — Diagnóstico host + guest (2026-10-16)

## HOST
- OpenGL: 4.3 Compat (Mesa 25.0.7, NVD9/nouveau GT 610)
- Vulkan: NÃO (hardware não suporta — Venus FORA, VirGL/OpenGL é o alvo)
- virglrenderer: 1.9.0 (`libvirglrenderer.so.1.9.0` presente)
- Mesa host: 25.x (dri: crocus/i915/iris/swrast presentes)
- QEMU: 10.0.13 com `virtio-vga-gl`, `virtio-gpu-gl-pci` — SUPORTA VirGL

## GUEST (OvsbOS)
- PCI: SIM (`OvsbMk/drivers/pci.c`, portas 0xCF8/0xCFC)
- virtio: SIM (`OvsbMk/drivers/virtio_gpu.c` 461 linhas + `.h`)
- Estado existente: descoberta PCI + caps modern + controlq(256) +
  features (aceita tudo, incl. VIRGL bit 0) + GET_DISPLAY_INFO +
  RESOURCE_CREATE_2D + ATTACH_BACKING + SET_SCANOUT + TRANSFER/FLUSH.
  Kernel tenta virtio primeiro, cai p/ VESA se falhar (`kernel.c`).
- Fases 2-4 = VALIDAR o que existe (não reescrever). Fases 5+ = implementar.
