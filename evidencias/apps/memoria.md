# Memoria dos apps base (medido no host Linux x86_64)

Metodo: cada app executado com entrada scriptada; pico de `VmRSS`
amostrado via `/proc/<pid>/status` durante a execucao.
(valores de referencia host — no guest OvsbOS o loader e outro,
mas a ordem de grandeza do working set vale).

| App | Binario (host dyn) | Binario (static p/ /BIN) | Pico RSS medido | BSS estatica principal |
|---|---|---|---|---|
| editor | 21640 B | 776448 B | 252 kB | buffer 512x512 B (256 kB) |
| paint | 21416 B | 894224 B | 244 kB | canvas 640x480x4 + undo x10 (~13,4 MB virtuais; residente so paginas tocadas) |
| fm | 23120 B | 797464 B | 1508 kB | tabela 1024 entradas + dbuf getdents 16 kB |
| shell (ovsh) | 22024 B | 788360 B | 252 kB | historico 64x1024 B (64 kB) |

Notas:
- Static glibc ~800 kB/binario: aceitavel (DOOM.ELF no /BIN tem 458 kB,
  XORG 2,6 MB). Alternativa futura: linkar contra a libc do userland
  OvsbOS (Mach-O) para binarios menores.
- `paint`: o anel de undo (10 niveis) e o maior consumidor virtual;
  sob memoria curta pode-se reduzir `UNDO_N` para 3 (define no topo).
- Zumbis: 0 apos 5x abre/fecha por app e apos rodada simultanea dos 3
  (verificacao via `ps ... defunct`).
- Simultaneidade host: editor+paint+shell juntos, todos rc=0.

## Integracao com o menu (Fase 3.1)

O menu INICIAR (`OvsbMk/tests/desktop/ovsb-desktop.c`, `scan_dir()`)
lista automaticamente todo ELF em `/BIN` e `/APPS` — sem cadastro manual.
Os 4 binarios estaticos foram instalados no `disk.img`:

- `/BIN/EDITOR` (776448 B)
- `/BIN/PAINT` (894224 B)
- `/BIN/FM` (797464 B)
- `/BIN/OVSH` (788360 B; nome evita colidir com o `/BIN/SH` existente)

Icones XPM/32x32+16x16 em `assets/icons/`:
`editor-*.xpm/png`, `paint-*.xpm/png`, `fm-*.xpm/png`, `shell-*.xpm/png`
(XPM pronto p/ toolkits X11/FVWM; PNG p/ previews e Wayland).
