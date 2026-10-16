# FASE 4 — 2D validado no hardware (2026-10-16)

QEMU: `-device virtio-vga-gl -display gtk,gl=on` (virglrenderer 1.9 host).
- PCI discovery: OK (`BAR 0xFE800000`, caps modern, nmult=4)
- Features `f0=0x30000013`: **VIRGL bit 0 ON**
- queues: controlq OK (256); DRIVER_OK
- GET_DISPLAY_INFO: **1280x800**; RESOURCE_CREATE_2D (B8G8R8X8) OK;
  ATTACH_BACKING OK; SET_SCANOUT 0 OK; `[VG] init OK!`
- TRANSFER_TO_HOST_2D + RESOURCE_FLUSH: OK (via `disp_flush` 201)

## FPS (`/BIN/VG_FPS`, retângulo fullscreen + flush/frame, 10s)
`vg_fps: 1280x800` / 80, 16, 6, 4, 3, 582 / `fim total=1613`
Média ≈ 160 fps (janela parcial do último segundo distorce; metodologia:
frames/tEMPO total). Gargalo: fill de 4MB em CPU + roundtrip de flush.

## Bug achado e corrigido no caminho
Syscalls TipOS-nativas via instrução `syscall` entregavam arg4 errado
(slot rcx = RIP de retorno em vez de r10): `disp_get_fb` escrevia
`pitch` em endereço selvagem (PF@0). Fix em `syscall_entry.asm`
(`push r10` no slot; ABI declara rcx clobbered). Validado: `mini: r=0
a=ffffffff80000000 w=500 h=320 p=1400`.
