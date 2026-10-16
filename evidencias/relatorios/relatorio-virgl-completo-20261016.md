# Relatório VirGL Completo — 2026-10-16 (branch `virgl/20261016-1138`)

## 1. Resumo executivo

Noite VirGL com resultado parcial sólido: **2D acelerado por virtio-gpu
funcionando de verdade** (descoberta PCI, VIRGL negociado, scanout
1280x800, flush, cursor de hardware, ~160 fps médios num fill fullscreen),
**`/dev/dri/card0` funcional** (version/caps/dumb/mmap/destroy/resources,
`drmtest` PASS com readback), e **quatro bugs reais achados e corrigidos
no caminho** (formato inválido, descritores sem reciclagem, arg4 no
trampolim, PDE 2MB mal-alinhada). **3D/Mesa não deu**: bloqueadores
arquiteturais documentados (sem pthreads/dlopen/wayland-client no
userland). QEMU morre intermitentemente sem deixar stderr (host-side,
não reproduzido a partir do guest; sessões SDL longas funcionam).

## 2. Host
- OpenGL 4.3 Compat, Mesa 25.0.7 (NVD9/nouveau, GT 610).
- Vulkan: NÃO (hardware não suporta — Venus fora, VirGL correto).
- virglrenderer 1.9.0 presente. QEMU 10.0.13 com `virtio-vga-gl`,
  `virtio-gpu-gl-pci`, `virtio-vga`, `virtio-gpu-pci`.

## 3. Guest
- PCI: sim (`drivers/pci.c`, 0xCF8/0xCFC). virtio: sim.
- Stack 2D pré-existente (461 linhas) foi VALIDADA, não reescrita.
- `/dev/dri/card0` (+`renderD128`, `virtio-gpu0`): novo (`kernel/drm.c`).

## 4. 2D: SIM
`f0=0x30000013` (VIRGL bit 0), display 1280x800, resource/attach/scanout/
transfer/flush OK. `/BIN/VG_FPS` (retângulo fullscreen + flush/frame):
80,16,6,4,3,582 → `fim total=1613` ≈ **160 fps médios** (gargalo: fill
4MB em CPU + roundtrip). Tela do usuário mostrou desktop (barra+cursor).

## 5. Cursor hardware: SIM (com ressalva)
cursorq(256) + resource 64x64 ARGB + UPDATE_CURSOR + MOVE_CURSOR por
movimento do mouse (dedup, fire-and-forget com reciclagem). Log:
`cursorq ok`, `cursor HW ok`. Ressalva: compositor ainda desenha o cursor
de software por cima (duplo); desligar o SW exige mexer no compositor.

## 6. `/dev/dri/card0`: ioctls implementados
VERSION, GET_CAP (DUMB_BUFFER/PREFERRED_DEPTH/PREFER_SHADOW/
TIMESTAMP_MONOTONIC), CREATE_DUMB, MAP_DUMB (+mmap com cookie
`0xD0000000`), DESTROY_DUMB, GETRESOURCES/CRTC/CONNECTOR/ENCODER,
ATOMIC no-op. VIRTGPU_* → `-ENOSYS` honesto com log.

## 7. Mesa: NÃO compilou (bloqueio real, §9)
## 8. Mesa drivers: idem
## 9. 3D: NÃO — bloqueadores
1. Sem pthreads (`u_thread` do Gallium), 2. sem dlopen (pipe-loader),
3. sem libwayland-client (EGL), 4. EXECBUFFER/CTX não implementados
(guest não tem quem gerar o stream). Ver `docs/virgl-fase-7-9-bloqueio.md`.
Ordem p/ desbloquear: pthreads → dlopen/static-monolito → wayland-client
→ libdrm → EXECBUFFER+TGSI. Estimativa: semanas.

## 10. Stress
Cliques HMP + rajadas de motion + scans: desktop sobrevive (fork-scan),
compositor e filhos morrem do bug `+0x68` conhecido (RIPs 0x08027378/
0x08027438/0x08028279 — endereço .text do kernel, varia por build).
Zero `SLAB-BAD`. QEMU morreu 3x sem stderr (sem correlação com atividade
guest; sessões SDL de 15min+ íntegras). **Bug pai-morto-em-syscall-comum
(pid 9) + compositor-morto = tela congelada total** (parece "QEMU crashou").

## 11. O que NÃO deu e próximos passos (5)
1. Triângulo 3D (bloqueado §9).
2. Root-cause do `+0x68` (evidência nova: RIP sempre em .text do kernel
   + frame iretq íntegro exceto slot RIP + stack saudável).
3. Validar preempção sem quebrar boot (revertida; ver NOTA-PREEMPCAO).
4. QEMU morrendo sem rastro (capturar stderr com `Report`/`dmesg` root).
5. Game Boy / JPEG / TCC (fora do escopo desta noite).

Evidências: `evidencias/virgl/shots/`, `docs/virgl-*.md`,
`evidencias/NOTA-*.md`, `evidencias/proposta-flicker-compositor.diff`.
