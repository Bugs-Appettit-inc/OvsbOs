# ESTADO — OvsbOS (branch `redondo/20261016-1446`)

Atualizado: 2026-10-16 (sessão "consolida").

## O que funciona
- Kernel OvsbMk (C+Zig+ASM): boot KVM, FAT32, ELF dinâmico+estático, syscalls Linux-compat.
- Compositor `WAYLAND_COMPOSITOR_LIB` + desktop `OVSB-DESK.OEP` (wallpaper, barra, relógio real, menu por categoria, janelas na barra 8.3, launcher 8.4).
- Formato `.oep` v1: header fixo de 128 bytes (`userspace/libs/oep/oep.h`) + ELF cru + SHA-256.
  - `oep-pack <elf> <nome> <categoria> -o APP.oep` (host + `/BIN/OEPPACK.OEP`).
  - `oep-info` (mostra nome/cat/hash e valida SHA-256), `oep-unpack`, `oep-run`.
  - Kernel: `execve` reconhece magic `OEP\0` e pula para `elf_offset` (ELF cru continua).
- Apps empacotados em `/BIN`: `EDITOR.OEP`, `PAINT.OEP`, `FM.OEP`, `SHELL.OEP`, `OEPPACK.OEP`, `OVSB-DESK.OEP` (ELF crus preservados).

## O que não funciona / pendente
- Verificação de SHA-256 no kernel (só nas ferramentas; kernel confia no pacote).
- triagem GPU: Mesa 3D bloqueado (fora de escopo), VirGL 2D ok (~9 FPS).
- Menu do desktop não tem teclado (layers não recebem teclado no compositor).
- `fm/main.c` restaurado de `39c0e3b` — edits de caça-bugs do `9d4ff25` perdidos (arquivo truncado no commit).

## Próximos passos
- Migrar restante do shell/menu p/ `.oep` (FOOT, DOOM cru ainda ELF).
- Verificação de hash opcional no kernel.

## Scripts QEMU
- `scripts/qemu.sh [vga|gl|gl256|tcg|headless]` — consórdio dos antigos (em `scripts/arquivo/`).
