# Estrutura de arquivos
```
OvsbMk/tests/desktop/ovsb-desktop.c   # cliente (wallpaper+barra+menu+launcher)
OvsbMk/tests/compositor/wayland_compositor_lib.c  # servidor (AUTORIZAÇÃO p/ tocar)
OvsbMk/tests/compositor/xdg-shell-*.{c,h}         # protocolo xdg
OvsbMk/tests/compositor/us_keymap_bin.c           # keymap
userspace/apps/{editor,paint,fm,shell,chip8}/     # apps (livres p/ mexer)
userspace/libs/image/                             # BMP/PPM/PNG (livre)
userspace/tools/{oep-pack,dev}/                   # packer + devtools (livres)
assets/icons/                                     # XPM/PNG 16+32
OvsbMk/kernel/{pit.c,process.c}                   # preempção (cuidado)
docs/desktop-fase-8.2/                            # ESTA DOC (congelada)
```

Build/instala desktop: `make -C OvsbMk build-ovsb-desktop`
(+ `build-wayland-lib` p/ o compositor). Binários em `/BIN` do `disk.img`
(menu auto-escaneia ELF). Teste de clique: QEMU + HMP
(`mouse_move`/`mouse_button`) + `tail serial.log`.
