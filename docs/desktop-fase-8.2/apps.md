# Apps do desktop (Fase 8.2)

Em `userspace/apps/`: `editor`, `paint`, `fm`, `shell` (+ `chip8`).
C99, `-Wall -Wextra -Werror`, standalone (só libc + syscalls; extensões
OvsbOS via syscall raw só com `OVSBOS=1`). Cada um tem Makefile próprio
(+ alvo `static` p/ `/BIN`) e modo `--gui` (framebuffer 200/201/202/206,
fonte 8x8 de `lib/gui/vesa.c`).

- **editor** (~552 loc): abrir/editar/salvar, setas/Home/End/PgUp/PgDn,
  Ctrl+S/F/Q/R, `--hl=c|asm|sh`, `--ln`, `--dump-img`, UTF-8 passthrough,
  binário preserva NUL.
- **paint** (~336 loc): `--cli new/line/rect/ell/fill/clear/undo/save/load`
  (PPM/BMP/PNG via `libs/image`), undo 10 níveis, GUI com mouse.
- **fm** (~374 loc): getdents64 raw, navega, mkdir/renomear/apagar (com
  confirmação), yank/colar-copia/move, abrir por extensão, preview `i`,
  status com espaço livre. Sanitiza nomes p/ `system()`.
- **shell** (~401 loc): builtins + pipe `|` + redir `> >> <` + aspas +
  histórico; `echo x > f` interno (sem `/bin/echo`).
- Instalados em `/BIN/{EDITOR,PAINT,FM,OVSH}` (menu auto-escaneia);
  ícones 16/32 em `assets/icons/`. RSS host: 250kB–1.5MB.
