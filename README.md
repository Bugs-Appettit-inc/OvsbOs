# OvsbOS v0.8.0

Sistema operacional x86-64 com kernel OvsbMk + desktop Wayland próprio
(ovsb-desktop): wallpaper, barra de tarefas, relógio, WM com titlebar
e terminal gráfico (foot).

```
/
├── OvsbMk/                 kernel OvsbMk (ring 0) + compositor Wayland
│   ├── kernel/             boot, memória, processos, syscalls, drivers
│   ├── kernel/drivers/     tty/pty, teclado, mouse
│   ├── fs/                 FAT32, ext2, initramfs
│   └── tests/
│       ├── compositor/     wayland_compositor_lib.c (servidor + WM)
│       ├── desktop/        ovsb-desktop.c (wallpaper + barra + relógio)
│       └── layer_shell_test.c
├── wl-build/               deps estáticas (wayland, pixman, harfbuzz,
│                           fcft, xkbcommon, ffi) — sem SIMD
├── src/                    userland (libc + graphy)
└── ovsb-term/              terminal alternativo
```

## Subsistemas

### OvsbMk — kernel (ring 0)
- Boot GRUB2 (Multiboot2), long mode 64-bit, PML4 identity mapping
- Syscalls via int 0x80, scheduler round-robin, fork/execve/waitpid
- VFS FAT32 + ext2, PTY/TTY com termios, `CLOCK_REALTIME` via RTC CMOS
- Compat Linux ELF (glibc dinâmico): `clone3`, `epoll`, `timerfd`,
  `memfd_create`, `SCM_RIGHTS`, `mmap` MAP_SHARED

### Compositor Wayland (`tests/compositor/`)
Servidor `libwayland-server` + `xdg_shell` + `wlr-layer-shell`:
- Globais: wl_compositor, wl_shm, wl_seat, xdg_wm_base,
  wl_subcompositor, wl_output, wl_data_device_manager,
  zwlr_layer_shell_v1
- WM: foco, z-order, mover (Alt+drag), resize (Alt+botão-direito),
  titlebar server-side com [X][_][□], Alt+Tab, purge-on-disconnect
- Input evdev → wl_pointer/wl_keyboard, cursor server-side
- Composição em 3 passes (bg/bottom → xdg → top/overlay);
  layers sem foco de teclado; pointer roteado por hierarquia

### ovsb-desktop (`tests/desktop/`)
Cliente Wayland (protocolo raw, sem libwayland no cliente):
- Layer background: wallpaper em gradiente 1280x800
- Layer top: barra 24px (botão "Iniciar" + relógio HH:MM:SS via
  `CLOCK_REALTIME`, exclusive_zone 24)
- Fonte bitmap 8x8 embutida

## Como buildar

Dependências (Debian/Ubuntu):
```
sudo apt install -y gcc make nasm zig qemu-system-x86 mtools \
  grub2-common xorriso
```

```bash
make -C OvsbMk build-wayland-lib   # compositor -> disk.img
make -C OvsbMk build-ovsb-desktop  # desktop -> disk.img
make -C OvsbMk                     # kernel
cp OvsbMk/build/kernel.elf OvsbMk/iso/boot/kernel.elf
grub-mkrescue -o OvsbOS.iso OvsbMk/iso
```

## Como rodar

```bash
qemu-system-x86_64 -enable-kvm -boot order=d -cdrom OvsbOS.iso \
  -m 512M -drive file=disk.img,format=raw,index=0 \
  -serial file:/tmp/ovsbos.log -rtc base=localtime -no-reboot
```
(`-rtc base=localtime` acerta o relógio da barra; sem ele o RTC
vem em UTC.)

O `disk.img` (FAT32) com `/BIN/*` é montado separado — gere com
`make -C OvsbMk disk.img` (alvo do Makefile interno) ou reuse um
existente com os binários (`FOOT`, `SH`, `WAYLAND_COMPOSITOR_LIB`,
`OVSB-DESKTOP`).

## Versão

Ver `VERSION`. Formato `v0.STAGE.RELEASE.FEATURE`:
- STAGE 8 = desktop Wayland sobre compositor + WM
