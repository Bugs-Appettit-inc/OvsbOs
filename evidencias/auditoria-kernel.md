# Auditoria de integridade do kernel — OvsbOS (noite/audit)

Metodologia: `gcc -fsyntax-only` p/ .c (NOTA: falhas por include-path freestanding, o build real usa -I próprio — NÃO são erros reais), `nasm -f elf64` p/ .asm (todos OK), varredura de padrões (strcpy/memcpy sem bound, TODO/FIXME, offsets mágicos, ausência de NULL-check, #if 0, declarações sem init). `utils.c`/`string.c` implementam strcpy/memcpy elas mesmas (falso-positivo). Criticidade ajustada manualmente nos achados abaixo.

Totais: 331 arquivos | OK=83 SUSPEITO=233 CRÍTICO(auto)=15

## Tabela por arquivo (selo automático)

| Arquivo | LOC | syntax | Selo | #achados |
|---|---|---|---|---|
| OvsbMk/drivers/ata.c | 137 | OK | SUSPEITO | 2 |
| OvsbMk/drivers/ata.h | 24 | N/A | SUSPEITO | 1 |
| OvsbMk/drivers/e1000.c | 130 | OK | SUSPEITO | 2 |
| OvsbMk/drivers/e1000.h | 42 | N/A | SUSPEITO | 1 |
| OvsbMk/drivers/keyboard.h | 28 | N/A | OK | 0 |
| OvsbMk/drivers/keyboard_asm.asm | 110 | OK | OK | 0 |
| OvsbMk/drivers/mouse.h | 27 | N/A | SUSPEITO | 1 |
| OvsbMk/drivers/pci.c | 25 | OK | SUSPEITO | 1 |
| OvsbMk/drivers/pci.h | 78 | N/A | SUSPEITO | 2 |
| OvsbMk/drivers/usb.c | 480 | OK | SUSPEITO | 1 |
| OvsbMk/drivers/usb.h | 19 | N/A | OK | 0 |
| OvsbMk/drivers/virtio_gpu.c | 461 | OK | SUSPEITO | 3 |
| OvsbMk/drivers/virtio_gpu.h | 144 | N/A | SUSPEITO | 2 |
| OvsbMk/fs/ext2.zig | 892 | N/A | SUSPEITO | 5 |
| OvsbMk/fs/ext2_new.zig | 731 | N/A | SUSPEITO | 6 |
| OvsbMk/fs/fat32.c | 1450 | OK | SUSPEITO | 4 |
| OvsbMk/fs/fat32.h | 121 | N/A | SUSPEITO | 2 |
| OvsbMk/fs/initramfs.zig | 118 | N/A | OK | 0 |
| OvsbMk/fs/vfs.c | 293 | OK | OK | 0 |
| OvsbMk/fs/vfs.h | 60 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/boot64.asm | 257 | OK | SUSPEITO | 8 |
| OvsbMk/kernel/console.c | 210 | OK | OK | 0 |
| OvsbMk/kernel/console.h | 34 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/drivers/keyboard.zig | 249 | N/A | OK | 0 |
| OvsbMk/kernel/drivers/mouse.zig | 201 | N/A | OK | 0 |
| OvsbMk/kernel/drivers/tty.c | 581 | FALHA | SUSPEITO | 1 |
| OvsbMk/kernel/drivers/tty.h | 129 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/dynlink.c | 453 | OK | SUSPEITO | 4 |
| OvsbMk/kernel/elf64.zig | 671 | N/A | SUSPEITO | 3 |
| OvsbMk/kernel/env.c | 112 | OK | CRÍTICO | 2 |
| OvsbMk/kernel/env.h | 41 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/idt.asm | 288 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/idt.c | 498 | OK | SUSPEITO | 12 |
| OvsbMk/kernel/idt.h | 46 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/kernel.c | 140 | OK | SUSPEITO | 4 |
| OvsbMk/kernel/mach_o.c | 343 | OK | SUSPEITO | 10 |
| OvsbMk/kernel/mach_o.h | 93 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/memory.c | 1281 | OK | SUSPEITO | 12 |
| OvsbMk/kernel/memory.h | 70 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/owt_app.c | 43 | OK | OK | 0 |
| OvsbMk/kernel/pic.c | 64 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/pit.c | 67 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/pit.h | 28 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/process.c | 1272 | OK | SUSPEITO | 8 |
| OvsbMk/kernel/process.h | 129 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/rtc.c | 57 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/rtc.h | 31 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/serial.c | 56 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/serial.h | 26 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/shell.c | 705 | OK | SUSPEITO | 5 |
| OvsbMk/kernel/shell.h | 21 | N/A | OK | 0 |
| OvsbMk/kernel/signal.c | 322 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/signal.h | 49 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/switch.asm | 252 | OK | SUSPEITO | 6 |
| OvsbMk/kernel/syscall.c | 5973 | OK | SUSPEITO | 12 |
| OvsbMk/kernel/syscall.h | 1711 | N/A | SUSPEITO | 2 |
| OvsbMk/kernel/syscall_entry.asm | 126 | OK | OK | 0 |
| OvsbMk/kernel/syscall_linux.zig | 403 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/tss.c | 114 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/tss.h | 24 | N/A | OK | 0 |
| OvsbMk/kernel/utils.c | 32 | OK | CRÍTICO | 4 |
| OvsbMk/kernel/utils.h | 29 | N/A | CRÍTICO | 2 |
| OvsbMk/kernel/vm_map.c | 778 | OK | SUSPEITO | 12 |
| OvsbMk/kernel/vm_map.h | 91 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/gui/vesa.c | 294 | FALHA | SUSPEITO | 2 |
| OvsbMk/lib/gui/vesa.h | 42 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt.h | 30 | N/A | OK | 0 |
| OvsbMk/lib/owt/owt_base.c | 61 | FALHA | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_base.h | 51 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_button.c | 76 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_button.h | 31 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_combobox.c | 93 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_combobox.h | 32 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_dialog.c | 69 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_dialog.h | 22 | N/A | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_draw.c | 113 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_draw.h | 23 | N/A | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_label.c | 45 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_label.h | 28 | N/A | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_listview.c | 88 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_listview.h | 33 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_menu.c | 91 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_menu.h | 32 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_statusbar.c | 45 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_statusbar.h | 28 | N/A | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_textbox.c | 79 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_textbox.h | 31 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_theme.c | 49 | OK | SUSPEITO | 1 |
| OvsbMk/lib/owt/owt_theme.h | 48 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/owt/owt_window.c | 63 | OK | OK | 0 |
| OvsbMk/lib/owt/owt_window.h | 31 | N/A | SUSPEITO | 2 |
| OvsbMk/lib/wm/wm.c | 67 | OK | SUSPEITO | 1 |
| OvsbMk/lib/wm/wm.h | 29 | N/A | SUSPEITO | 1 |
| OvsbMk/tests/compositor/us_keymap_bin.c | 4051 | OK | OK | 0 |
| OvsbMk/tests/compositor/wayland_compositor_lib.c | 1659 | OK | SUSPEITO | 12 |
| OvsbMk/tests/compositor/wayland_xdg_client.c | 205 | OK | CRÍTICO | 2 |
| OvsbMk/tests/compositor/xdg-shell-client-protocol.h | 2381 | N/A | OK | 0 |
| OvsbMk/tests/compositor/xdg-shell-protocol.c | 184 | OK | OK | 0 |
| OvsbMk/tests/compositor/xdg-shell-server-protocol.h | 2327 | N/A | OK | 0 |
| OvsbMk/tests/desktop/ovsb-desktop.c | 549 | OK | CRÍTICO | 9 |
| OvsbMk/tests/dyn_test.c | 25 | OK | OK | 0 |
| OvsbMk/tests/epoll_test.c | 54 | OK | OK | 0 |
| OvsbMk/tests/fase1_fc.c | 74 | FALHA | SUSPEITO | 1 |
| OvsbMk/tests/fase1_ftruncate.c | 29 | OK | OK | 0 |
| OvsbMk/tests/fase1_ftruncate_c.c | 71 | OK | SUSPEITO | 1 |
| OvsbMk/tests/fase1_mmap_ttf.c | 42 | OK | OK | 0 |
| OvsbMk/tests/fase1_stat.c | 43 | OK | SUSPEITO | 1 |
| OvsbMk/tests/fase1_xkb.c | 29 | OK | SUSPEITO | 1 |
| OvsbMk/tests/fb_evdev_test.c | 110 | OK | OK | 0 |
| OvsbMk/tests/fork_simple.c | 5 | OK | OK | 0 |
| OvsbMk/tests/hello.c | 15 | OK | SUSPEITO | 2 |
| OvsbMk/tests/layer_shell_test.c | 207 | OK | CRÍTICO | 7 |
| OvsbMk/tests/libtest.c | 4 | OK | OK | 0 |
| OvsbMk/tests/memfd_fork.c | 29 | OK | SUSPEITO | 4 |
| OvsbMk/tests/memfd_raw.c | 87 | OK | SUSPEITO | 3 |
| OvsbMk/tests/memfd_raw_scm.c | 75 | OK | SUSPEITO | 3 |
| OvsbMk/tests/memfd_simple.c | 25 | OK | SUSPEITO | 3 |
| OvsbMk/tests/memfd_test.c | 101 | OK | SUSPEITO | 2 |
| OvsbMk/tests/pty_test.c | 127 | OK | SUSPEITO | 1 |
| OvsbMk/tests/raw_fork.c | 24 | OK | SUSPEITO | 1 |
| OvsbMk/tests/sigchld_test.c | 63 | OK | OK | 0 |
| OvsbMk/tests/signal_test.c | 63 | OK | SUSPEITO | 1 |
| OvsbMk/tests/sigwinch_test.c | 84 | OK | SUSPEITO | 1 |
| OvsbMk/tests/socktest.c | 103 | OK | SUSPEITO | 2 |
| OvsbMk/tests/ttest.c | 152 | OK | SUSPEITO | 2 |
| OvsbMk/tests/ttest50.c | 96 | OK | SUSPEITO | 2 |
| OvsbMk/tests/wayland_client.c | 209 | OK | CRÍTICO | 6 |
| OvsbMk/tests/wayland_compositor.c | 212 | OK | CRÍTICO | 6 |
| OvsbMk/tests/wayland_compositor_shm.c | 435 | OK | CRÍTICO | 6 |
| OvsbMk/tests/wayland_shm_client.c | 234 | OK | CRÍTICO | 8 |
| src/userland/include/ctype.h | 50 | N/A | OK | 0 |
| src/userland/include/errno.h | 17 | N/A | OK | 0 |
| src/userland/include/fcntl.h | 11 | N/A | SUSPEITO | 1 |
| src/userland/include/inttypes.h | 20 | N/A | OK | 0 |
| src/userland/include/limits.h | 16 | N/A | OK | 0 |
| src/userland/include/math.h | 15 | N/A | OK | 0 |
| src/userland/include/stdbool.h | 7 | N/A | OK | 0 |
| src/userland/include/stdint.h | 27 | N/A | OK | 0 |
| src/userland/include/stdio.h | 90 | N/A | CRÍTICO | 3 |
| src/userland/include/stdlib.h | 43 | N/A | OK | 0 |
| src/userland/include/string.h | 47 | N/A | CRÍTICO | 4 |
| src/userland/include/strings.h | 7 | N/A | SUSPEITO | 1 |
| src/userland/include/sys/stat.h | 38 | N/A | SUSPEITO | 1 |
| src/userland/include/sys/time.h | 6 | N/A | SUSPEITO | 1 |
| src/userland/include/sys/types.h | 11 | N/A | OK | 0 |
| src/userland/include/tui.h | 151 | N/A | SUSPEITO | 2 |
| src/userland/include/unistd.h | 12 | N/A | SUSPEITO | 1 |
| src/userland/libc/crt0.c | 35 | OK | OK | 0 |
| src/userland/libc/stdio.c | 522 | FALHA | CRÍTICO | 4 |
| src/userland/libc/stdlib.c | 280 | OK | SUSPEITO | 1 |
| src/userland/libc/string.c | 221 | OK | CRÍTICO | 3 |
| src/userland/libc/tui.c | 721 | FALHA | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/am_map.c | 1355 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/am_map.h | 49 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/config.h | 100 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_englsh.h | 693 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_event.c | 63 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_event.h | 137 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/d_items.c | 128 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_items.h | 41 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_iwad.c | 848 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/d_iwad.h | 52 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_loop.c | 826 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/d_loop.h | 81 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_main.c | 1845 | OK | SUSPEITO | 12 |
| src/userland/progs/doom/doomgeneric/d_main.h | 50 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_mode.c | 209 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_mode.h | 98 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_net.c | 281 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_player.h | 209 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_textur.h | 43 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/d_think.h | 68 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/d_ticcmd.h | 56 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/deh_main.h | 48 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/deh_misc.h | 83 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/deh_str.h | 47 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/doom.h | 42 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomdata.h | 213 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomdef.c | 28 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomdef.h | 168 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomfeatures.h | 40 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/doomgeneric.c | 27 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomgeneric.h | 51 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/doomkeys.h | 97 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomstat.c | 35 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/doomstat.h | 281 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/doomtype.h | 104 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/dstrings.c | 73 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/dstrings.h | 41 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/f_finale.c | 718 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/f_finale.h | 45 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/f_wipe.c | 294 | OK | SUSPEITO | 6 |
| src/userland/progs/doom/doomgeneric/f_wipe.h | 63 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/g_game.c | 2303 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/g_game.h | 80 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/gusconf.c | 271 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/gusconf.h | 29 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/hu_lib.c | 347 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/hu_lib.h | 182 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/hu_stuff.c | 641 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/hu_stuff.h | 59 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_cdmus.h | 41 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/i_endoom.h | 29 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_input.c | 360 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_joystick.h | 70 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/i_scale.c | 1452 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/i_scale.h | 53 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_sound.c | 420 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_sound.h | 256 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/i_swap.h | 73 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/i_system.c | 578 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/i_system.h | 84 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/i_timer.c | 96 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/i_timer.h | 42 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/i_video.c | 495 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/i_video.h | 175 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/icon.c | 262 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/info.c | 4662 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/info.h | 1331 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_argv.c | 265 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/m_argv.h | 49 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_bbox.c | 54 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_bbox.h | 47 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_cheat.c | 89 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/m_cheat.h | 62 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/m_config.c | 2128 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_config.h | 39 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_controls.c | 398 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/m_controls.h | 168 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/m_fixed.c | 62 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/m_fixed.h | 39 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/m_menu.c | 2125 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/m_menu.h | 61 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_misc.c | 536 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_misc.h | 51 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/m_random.c | 65 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/m_random.h | 39 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/memio.c | 197 | OK | SUSPEITO | 5 |
| src/userland/progs/doom/doomgeneric/memio.h | 38 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/mus2mid.c | 737 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/mus2mid.h | 9 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_client.h | 52 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_dedicated.h | 25 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/net_defs.h | 248 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_gui.h | 29 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/net_io.h | 36 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_loop.h | 27 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/net_packet.h | 44 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_query.h | 44 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/net_sdl.h | 26 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/net_server.h | 42 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_ceilng.c | 324 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_doors.c | 778 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_enemy.c | 2006 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_floor.c | 546 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_inter.c | 922 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_inter.h | 30 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_lights.c | 350 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_local.h | 297 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/p_map.c | 1448 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_maputl.c | 1001 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_mobj.c | 1049 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/p_mobj.h | 284 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_plats.c | 304 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_pspr.c | 888 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_pspr.h | 71 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_saveg.c | 1891 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_saveg.h | 62 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_setup.c | 855 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_setup.h | 37 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/p_sight.c | 350 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_spec.c | 1489 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/p_spec.h | 637 | N/A | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/p_switch.c | 648 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/p_telept.c | 133 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/p_tick.c | 151 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/p_tick.h | 33 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/p_user.c | 379 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/r_bsp.c | 573 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/r_bsp.h | 61 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_data.c | 912 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/r_data.h | 51 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_defs.h | 448 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_draw.c | 975 | OK | SUSPEITO | 5 |
| src/userland/progs/doom/doomgeneric/r_draw.h | 104 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/r_local.h | 45 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/r_main.c | 891 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/r_main.h | 160 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_plane.c | 446 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/r_plane.h | 76 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_segs.c | 743 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/r_segs.h | 33 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_sky.c | 52 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_sky.h | 37 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/r_state.h | 127 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/r_things.c | 982 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/r_things.h | 65 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/s_sound.c | 670 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/s_sound.h | 89 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/sha1.h | 40 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/sounds.c | 229 | OK | OK | 0 |
| src/userland/progs/doom/doomgeneric/sounds.h | 227 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/st_lib.c | 284 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/st_lib.h | 209 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/st_stuff.c | 1416 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/st_stuff.h | 89 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/statdump.h | 23 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/tables.c | 2227 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/tables.h | 96 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/v_patch.h | 50 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/v_video.c | 932 | OK | SUSPEITO | 6 |
| src/userland/progs/doom/doomgeneric/v_video.h | 108 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_checksum.c | 87 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_checksum.h | 27 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/w_file.c | 95 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_file.h | 78 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/w_file_stdc.c | 96 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_main.c | 198 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_main.h | 24 | N/A | OK | 0 |
| src/userland/progs/doom/doomgeneric/w_merge.h | 44 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/w_wad.c | 612 | OK | SUSPEITO | 3 |
| src/userland/progs/doom/doomgeneric/w_wad.h | 78 | N/A | SUSPEITO | 2 |
| src/userland/progs/doom/doomgeneric/wi_stuff.c | 1829 | OK | SUSPEITO | 4 |
| src/userland/progs/doom/doomgeneric/wi_stuff.h | 48 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/z_zone.c | 488 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/doomgeneric/z_zone.h | 73 | N/A | SUSPEITO | 1 |
| src/userland/progs/doom/fbtest.c | 34 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/tipOS_doom.c | 15 | OK | SUSPEITO | 1 |
| src/userland/progs/doom/tipOS_glue.c | 235 | OK | SUSPEITO | 2 |
| src/userland/progs/doom/tipOS_stubs.c | 66 | OK | SUSPEITO | 1 |
| src/userland/progs/graphy.c | 739 | FALHA | CRÍTICO | 5 |

## Detalhes SUSPEITO/CRÍTICO (top: kernel + fs + lib + drivers)

### OvsbMk/drivers/ata.c — SUSPEITO (syntax=OK)
- 2 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/ata.h — SUSPEITO (syntax=N/A)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/e1000.c — SUSPEITO (syntax=OK)
- L129: marcador (return -1; // TODO)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/e1000.h — SUSPEITO (syntax=N/A)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/mouse.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/pci.c — SUSPEITO (syntax=OK)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/pci.h — SUSPEITO (syntax=N/A)
- 1 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/drivers/usb.c — SUSPEITO (syntax=OK)
- 4 declaracao(oes) sem inicializacao

### OvsbMk/drivers/virtio_gpu.c — SUSPEITO (syntax=OK)
- L114: aritmetica ponteiro/offset magico (total = (total + 0xFFF) & ~0xFFF;)
- L358: aritmetica ponteiro/offset magico (uint32_t pages = (uint32_t)((fb_sz + 0xFFF) / 0x1000);)
- 15 declaracao(oes) sem inicializacao

### OvsbMk/drivers/virtio_gpu.h — SUSPEITO (syntax=N/A)
- 23 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/fs/ext2.zig — SUSPEITO (syntax=N/A)
- L500: marcador (// [TODO] Lista entradas de um diretório. Por enquanto é só um placeho)
- L508: marcador (return 0; // TODO: implement directory listing)
- L568: memcpy sem bound visivel (@memcpy(gd_fresh[gd_off .. gd_off + 32], bytes[0..32]);)
- L645: memcpy sem bound visivel (@memcpy(gd_fresh[gd_off .. gd_off + 32], bytes[0..32]);)
- L672: memcpy sem bound visivel (@memcpy(dst[off .. off + 128], src[0..128]);)

### OvsbMk/fs/ext2_new.zig — SUSPEITO (syntax=N/A)
- L420: memcpy sem bound visivel (@memcpy(buf[done..done + chunk], blk[in_off..in_off + chunk]);)
- L448: memcpy sem bound visivel (@memcpy(buf[done..done + chunk], blk[in_off..in_off + chunk]);)
- L489: memcpy sem bound visivel (@memcpy(out.name[0..copy_len], block[name_start..name_start + copy_len)
- L517: memcpy sem bound visivel (@memcpy(blk[in_off..in_off + chunk], data[done..done + chunk]);)
- L608: memcpy sem bound visivel (@memcpy(blk[noff + 8 .. noff + 8 + name.len], name);)
- L629: memcpy sem bound visivel (@memcpy(blk[8 .. 8 + name.len], name);)

### OvsbMk/fs/fat32.c — SUSPEITO (syntax=OK)
- L124: aritmetica ponteiro/offset magico (return *(uint32_t *)(fat_cache + ent_offset) & 0x0FFFFFFF;)
- L135: aritmetica ponteiro/offset magico ((*(uint32_t *)(sector + ent_offset) & 0xF0000000) | (value & 0x0FFFFFF)
- L311: marcador (/* ~~ DEBUG temporario: mostra busca vs nome real ~~ */)
- 2 declaracao(oes) sem inicializacao

### OvsbMk/fs/fat32.h — SUSPEITO (syntax=N/A)
- 39 declaracao(oes) sem inicializacao
- 15 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/fs/vfs.h — SUSPEITO (syntax=N/A)
- 4 declaracao(oes) sem inicializacao
- 13 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/boot64.asm — SUSPEITO (syntax=OK)
- L81: aritmetica ponteiro/offset magico (; Entrada 0 da PML4 → PDP[0] → PD[0..511] cobre endereços 0x00000000-0)
- L217: aritmetica ponteiro/offset magico (; ♥ Page tables 4-level paging (identity map 0x00000000 - 0x3FFFFFFF) )
- L223: marcador (;   Antes era 0x87 (com U): TODO o RAM baixo (inclusive frames livres)
- L229: aritmetica ponteiro/offset magico (; Mapeamento: PD[i] → endereço (i * 0x200000) + 0x83~)
- L230: aritmetica ponteiro/offset magico (;   PD[0] → 0x00000000 + 0x83 (primeiro 2MB: 0x000000-0x1FFFFF)~)
- L231: aritmetica ponteiro/offset magico (;   PD[1] → 0x00200000 + 0x83 (2MB: 0x200000-0x3FFFFF)~)
- L232: aritmetica ponteiro/offset magico (;   ... até PD[511] → 0xFFE00000 + 0x83 (último 2MB do 1GB)~)
- L245: aritmetica ponteiro/offset magico (dq (i * 0x200000) + 0x83    ; 0x83 = Present | R/W | PS (2MB, supervis)

### OvsbMk/kernel/console.h — SUSPEITO (syntax=N/A)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/drivers/tty.c — SUSPEITO (syntax=FALHA)
- syntax: OvsbMk/kernel/drivers/tty.c:3:10: fatal error: serial.h: Arquivo ou diretório inexistente | 3 | #include "serial.h" | |          ^~~~~~~~~~ | compilation terminated.

### OvsbMk/kernel/drivers/tty.h — SUSPEITO (syntax=N/A)
- 20 declaracao(oes) sem inicializacao
- 17 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/dynlink.c — SUSPEITO (syntax=OK)
- L65: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(e & 0x000FFFFFFFFFF000ULL);)
- L69: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(e2 & 0x000FFFFFFFFFF000ULL);)
- L73: aritmetica ponteiro/offset magico (uint64_t *pt = (uint64_t *)(e3 & 0x000FFFFFFFFFF000ULL);)
- 10 declaracao(oes) sem inicializacao

### OvsbMk/kernel/elf64.zig — SUSPEITO (syntax=N/A)
- L402: aritmetica ponteiro/offset magico (const extra_end = (dst_va + phdr.p_memsz + 0xFFF) & ~@as(u64, 0xFFF);)
- L472: aritmetica ponteiro/offset magico (const aligned_end = (seg_end + 0x1FFFFF) & ~@as(u64, 0x1FFFFF);)
- L557: aritmetica ponteiro/offset magico (const aligned_end = (seg_end + 0x1FFFFF) & ~@as(u64, 0x1FFFFF);)

### OvsbMk/kernel/env.c — CRÍTICO (syntax=OK)
- L91: copia sem bound (strcpy(alias_name[j], alias_name[j+1]);)
- L92: copia sem bound (strcpy(alias_val[j], alias_val[j+1]);)

### OvsbMk/kernel/env.h — SUSPEITO (syntax=N/A)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/idt.asm — SUSPEITO (syntax=OK)
- L237: marcador (; Salvamos TODOS os 15 GPRs (rax..r15) pra C poder mexer sem medo~)

### OvsbMk/kernel/idt.c — SUSPEITO (syntax=OK)
- L209: aritmetica ponteiro/offset magico (serial_puts(" stride="); serial_puthex(*(uint32_t*)0x10014510);)
- L210: aritmetica ponteiro/offset magico (serial_puts(" scr_w="); serial_puthex(*(uint32_t*)0x10014518);)
- L211: aritmetica ponteiro/offset magico (serial_puts(" scr_h="); serial_puthex(*(uint32_t*)0x10014514);)
- L212: aritmetica ponteiro/offset magico (serial_puts(" bb="); serial_puthex(*(uint64_t*)0x10014520);)
- L286: aritmetica ponteiro/offset magico (uint64_t *p3 = (uint64_t *)(uintptr_t)(p4[pi] & ~0xFFFULL);)
- L292: aritmetica ponteiro/offset magico (uint64_t *p2 = (uint64_t *)(uintptr_t)(p3[qi] & ~0xFFFULL);)
- L298: aritmetica ponteiro/offset magico (uint64_t *p1 = (uint64_t *)(uintptr_t)(p2[di] & ~0xFFFULL);)
- L336: aritmetica ponteiro/offset magico (uint64_t *q3 = (uint64_t *)(uintptr_t)(q4[pi2] & ~0xFFFULL);)
- L338: aritmetica ponteiro/offset magico (uint64_t *q2 = (uint64_t *)(uintptr_t)(q3[qi2] & ~0xFFFULL);)
- L365: aritmetica ponteiro/offset magico (uint64_t *q1 = (uint64_t *)(uintptr_t)(q2[di2] & ~0xFFFULL);)

### OvsbMk/kernel/idt.h — SUSPEITO (syntax=N/A)
- 9 declaracao(oes) sem inicializacao

### OvsbMk/kernel/kernel.c — SUSPEITO (syntax=OK)
- L95: aritmetica ponteiro/offset magico (*(uint64_t *)0x500000 = g_fb.addr;)
- L96: aritmetica ponteiro/offset magico (*(uint32_t *)0x500008 = g_fb.width;)
- L97: aritmetica ponteiro/offset magico (*(uint32_t *)0x50000C = g_fb.height;)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/mach_o.c — SUSPEITO (syntax=OK)
- L66: aritmetica ponteiro/offset magico (uint64_t extra_end = (seg_end + 0x1BFFF) & ~0xFFFULL;)
- L135: aritmetica ponteiro/offset magico (uint64_t *old_pdp = (uint64_t *)(uintptr_t)(old_pml4[0] & ~0xFFFULL);)
- L136: aritmetica ponteiro/offset magico (uint64_t *old_pd = (uint64_t *)(uintptr_t)(old_pdp[0] & ~0xFFFULL);)
- L154: aritmetica ponteiro/offset magico (* congelava — RIP=kmalloc+0x149, PD[0x12]=0). Clona a PT pro)
- L157: aritmetica ponteiro/offset magico (uint64_t *old_pt = (uint64_t *)(uintptr_t)(pde & ~0xFFFULL);)
- L197: aritmetica ponteiro/offset magico (uint64_t *source_pd = (uint64_t *)(uintptr_t)(pdp_entry & ~0xFFFULL);)
- L203: aritmetica ponteiro/offset magico (uint64_t *source_pt = (uint64_t *)(uintptr_t)(pde & ~0xFFFULL);)
- L253: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & ~0xFFFULL);)
- L255: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFFULL);)
- L294: aritmetica ponteiro/offset magico (uint64_t aligned_end = (seg_end + 0x1FFFFF) & ~(0x1FFFFFULL);)

### OvsbMk/kernel/mach_o.h — SUSPEITO (syntax=N/A)
- 28 declaracao(oes) sem inicializacao

### OvsbMk/kernel/memory.c — SUSPEITO (syntax=OK)
- L220: marcador (/* varre TODOS os processos (não só o CR3 atual) */)
- L230: aritmetica ponteiro/offset magico (uint64_t *p3 = (uint64_t *)(uintptr_t)(e4 & 0x000FFFFFFFFFF000ULL);)
- L234: aritmetica ponteiro/offset magico (uint64_t *p2 = (uint64_t *)(uintptr_t)(e3 & 0x000FFFFFFFFFF000ULL);)
- L240: aritmetica ponteiro/offset magico (target < ((e2 & 0x000FFFFFE00000ULL) + 0x200000)) {)
- L248: aritmetica ponteiro/offset magico (uint64_t *p1 = (uint64_t *)(uintptr_t)(e2 & 0x000FFFFFFFFFF000ULL);)
- L350: marcador (/* FASE5: marca ALLOC|USER em TODOS os frames do bloco (ordem só na)
- L655: aritmetica ponteiro/offset magico (uint64_t kend = ((uint64_t)(uintptr_t)__bss_end + 0xFFFULL) & ~0xFFFUL)
- L939: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & ~0xFFFULL);)
- L941: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFFULL);)
- L952: aritmetica ponteiro/offset magico (uint64_t *pt = (uint64_t *)(uintptr_t)(pd[pd_idx] & ~0xFFFULL);)

### OvsbMk/kernel/memory.h — SUSPEITO (syntax=N/A)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/pic.c — SUSPEITO (syntax=OK)
- 1 declaracao(oes) sem inicializacao

### OvsbMk/kernel/pit.c — SUSPEITO (syntax=OK)
- 1 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/pit.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/process.c — SUSPEITO (syntax=OK)
- L222: aritmetica ponteiro/offset magico (*(--sp) = 0x202;)
- L803: aritmetica ponteiro/offset magico (uint64_t *p3s = (uint64_t *)(uintptr_t)(p4s[i4] & ~0xFFFULL);)
- L811: aritmetica ponteiro/offset magico (uint64_t *p2s = (uint64_t *)(uintptr_t)(p3s[i3] & ~0xFFFULL);)
- L838: aritmetica ponteiro/offset magico (uint64_t *sp3 = (uint64_t *)(uintptr_t)(e4 & 0x000FFFFFFFFFF000ULL);)
- L843: aritmetica ponteiro/offset magico (uint64_t *sp2 = (uint64_t *)(uintptr_t)(e3 & 0x000FFFFFFFFFF000ULL);)
- L853: aritmetica ponteiro/offset magico (*  (VA 0xFE800000 sem identity garantido)~ */)
- L872: aritmetica ponteiro/offset magico (uint64_t srcv = *(uint64_t *)(uintptr_t)(pa2 + 0x818);)
- L873: aritmetica ponteiro/offset magico (uint64_t dstv = *(uint64_t *)(uintptr_t)(fr + 0x818);)

### OvsbMk/kernel/process.h — SUSPEITO (syntax=N/A)
- 19 declaracao(oes) sem inicializacao
- 8 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/rtc.c — SUSPEITO (syntax=OK)
- 1 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/rtc.h — SUSPEITO (syntax=N/A)
- 1 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/serial.c — SUSPEITO (syntax=OK)
- 1 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/serial.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/shell.c — SUSPEITO (syntax=OK)
- L250: aritmetica ponteiro/offset magico (uint64_t a_end = (elf_base + seg_end + 0x1FFFFF) & ~(0x1FFFFFULL);)
- L264: aritmetica ponteiro/offset magico (elf_bss_end = (elf_bss_end + 0xFFF) & ~0xFFFULL;)
- L402: marcador (/* varre TODOS os pares do auxv (do início ao AT_NULL)~~ */)
- L698: marcador (/* ♥ TODO: adicionar suporte a argumentos pra exec ~ argv futuramente!)
- 4 declaracao(oes) sem inicializacao

### OvsbMk/kernel/signal.c — SUSPEITO (syntax=OK)
- 6 declaracao(oes) sem inicializacao
- 10 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/signal.h — SUSPEITO (syntax=N/A)
- 4 declaracao(oes) sem inicializacao
- 5 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/switch.asm — SUSPEITO (syntax=OK)
- L32: marcador (;   a partir de MOV CR3, rax TODOS os endereços são reinterpretados~)
- L79: marcador (; DEBUG K removed)
- L99: marcador (; DEBUG N removed)
- L141: marcador (; DEBUG R removed)
- L162: marcador (; DEBUG r removed)
- L198: marcador (; DEBUG U removed)

### OvsbMk/kernel/syscall.c — SUSPEITO (syntax=OK)
- L156: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(uintptr_t)(pml4e & ~0xFFFULL);)
- L162: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(uintptr_t)(pdpte & ~0xFFFULL);)
- L168: aritmetica ponteiro/offset magico (uint64_t *pt = (uint64_t *)(uintptr_t)(pde & ~0xFFFULL);)
- L1026: aritmetica ponteiro/offset magico (size_t need_pages = (size_t)((off+(uint64_t)count+0xFFF)>>12);)
- L1043: aritmetica ponteiro/offset magico (for(int i=0;i<count;i++){ size_t pg=(size_t)((off+(uint64_t)i)>>12); s)
- L1383: marcador (* Varre as fd tables de TODOS os processos vivos~ */)
- L1713: marcador (* readv com 2 iovecs — sem handler TODO fread retornava)
- L1839: marcador (/* FASE6-evdev: pacote SÓ é consumido se TODOS os seus)
- L2917: aritmetica ponteiro/offset magico (fb_size = (fb_size + 0x1FFFFF) & ~0x1FFFFFULL;)
- L3023: aritmetica ponteiro/offset magico (uint64_t end = addr + ((len + 0xFFF) & ~0xFFFULL);)

### OvsbMk/kernel/syscall.h — SUSPEITO (syntax=N/A)
- 1 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/syscall_linux.zig — SUSPEITO (syntax=N/A)
- L342: marcador (// (231 é tipo um numero de onibus que TODO)

### OvsbMk/kernel/tss.c — SUSPEITO (syntax=OK)
- 15 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/utils.c — CRÍTICO (syntax=OK)
- L21: copia sem bound (char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s+)
- L24: memcpy sem bound visivel (void *memcpy(void *d, const void *s, int n) { for (int i = 0; i < n; i)
- L25: memcpy sem bound visivel (void *memmove(void *d, const void *s, int n) { return memcpy(d, s, n);)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/utils.h — CRÍTICO (syntax=N/A)
- L19: copia sem bound (char *strcpy(char *d, const char *s);)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/kernel/vm_map.c — SUSPEITO (syntax=OK)
- L63: aritmetica ponteiro/offset magico (int npages = (size + 0xFFF) >> 12;)
- L232: aritmetica ponteiro/offset magico (size = (size + 0xFFF) & ~0xFFFULL;)
- L238: aritmetica ponteiro/offset magico (base = (base + 0xFFF) & ~0xFFFULL;)
- L276: aritmetica ponteiro/offset magico (size = (size + 0xFFF) & ~0xFFFULL;)
- L421: aritmetica ponteiro/offset magico (uint64_t a_end   = (we + 0x1FFFFF) & ~0x1FFFFFULL;)
- L433: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(uintptr_t)(p4[pml4_idx] & ~0xFFFULL);)
- L440: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFFULL);)
- L474: aritmetica ponteiro/offset magico (pt = (uint64_t *)(uintptr_t)(old & ~0xFFFULL);)
- L536: aritmetica ponteiro/offset magico (uint64_t *pdpt = (uint64_t *)(uintptr_t)(p4[pml4_idx] & ~0xFFFULL);)
- L538: aritmetica ponteiro/offset magico (uint64_t *pd = (uint64_t *)(uintptr_t)(pdpt[pdpt_idx] & ~0xFFFULL);)

### OvsbMk/kernel/vm_map.h — SUSPEITO (syntax=N/A)
- 10 declaracao(oes) sem inicializacao
- 12 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/gui/vesa.c — SUSPEITO (syntax=FALHA)
- syntax: OvsbMk/lib/gui/vesa.c:15:10: fatal error: ../kernel/memory.h: Arquivo ou diretório inexistente | 15 | #include "../kernel/memory.h" | |          ^~~~~~~~~~~~~~~~~~~~ | compilation terminated.
- L142: aritmetica ponteiro/offset magico (fb_size = (fb_size + 0x1FFFFF) & ~0x1FFFFF;)

### OvsbMk/lib/gui/vesa.h — SUSPEITO (syntax=N/A)
- 5 declaracao(oes) sem inicializacao
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_base.c — SUSPEITO (syntax=FALHA)
- syntax: OvsbMk/lib/owt/owt_base.c:20:10: fatal error: ../kernel/memory.h: Arquivo ou diretório inexistente | 20 | #include "../kernel/memory.h" | |          ^~~~~~~~~~~~~~~~~~~~ | compilation terminated.
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_base.h — SUSPEITO (syntax=N/A)
- 3 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_button.c — SUSPEITO (syntax=OK)
- 4 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_button.h — SUSPEITO (syntax=N/A)
- 2 declaracao(oes) sem inicializacao
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_combobox.c — SUSPEITO (syntax=OK)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_combobox.h — SUSPEITO (syntax=N/A)
- 3 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_dialog.c — SUSPEITO (syntax=OK)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_dialog.h — SUSPEITO (syntax=N/A)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_draw.c — SUSPEITO (syntax=OK)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_draw.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_label.c — SUSPEITO (syntax=OK)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_label.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_listview.c — SUSPEITO (syntax=OK)
- 4 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_listview.h — SUSPEITO (syntax=N/A)
- 3 declaracao(oes) sem inicializacao
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_menu.c — SUSPEITO (syntax=OK)
- 4 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_menu.h — SUSPEITO (syntax=N/A)
- 3 declaracao(oes) sem inicializacao
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_statusbar.c — SUSPEITO (syntax=OK)
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_statusbar.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_textbox.c — SUSPEITO (syntax=OK)
- 3 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_textbox.h — SUSPEITO (syntax=N/A)
- 2 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_theme.c — SUSPEITO (syntax=OK)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_theme.h — SUSPEITO (syntax=N/A)
- 10 declaracao(oes) sem inicializacao
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/owt/owt_window.h — SUSPEITO (syntax=N/A)
- 1 declaracao(oes) sem inicializacao
- 2 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/wm/wm.c — SUSPEITO (syntax=OK)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

### OvsbMk/lib/wm/wm.h — SUSPEITO (syntax=N/A)
- 1 funcao(oes) c/ ptr sem checagem NULL visivel

---

## Achados manuais (revisão dirigida — pontos cegos do bug +0x68)

### A1. [CRÍTICO] `vm_mprotect` sem `invlpg` — TLB stale após troca de NX/RW
- Arquivo: `OvsbMk/kernel/vm_map.c:556-573` (`pte_set_prot`) + `689-704` (`vm_mprotect`).
- O callback altera bits W (0x02), U (0x04) e **NX (bit 63)** das PTEs e retorna
  sem nenhum `invlpg`; `vm_mprotect` também não tem loop de flush (compare com
  `vm_munmap`, que TEM loop `invlpg` + `__sync_synchronize`, e `pte_absent`, que
 TEM `invlpg`).
- Efeito: após `mprotect` (RELRO do loader, transições W->X de JIT), a TLB
  continua aplicando as permissões ANTIGAS. Execução numa página cuja TLB ainda
  diz NX gera `#PF err=0x15` (P=1,U=1,I=1) mesmo com as tabelas corretas —
  EXATAMENTE a assinatura do crash em `evidencias/bug2-rip-dump.log.bak`
  (`PF@0x08027478 ... err=0x15`, tabelas `PD[0x40]=0x080000E3` presente+user,
  `PDhi=0` sem NX).
- Flakiness explicada: depende de a entrada TLB estar ou não cacheada/evictada
  (~50%/clique sob churn de mmap/mprotect do loader+menu, 0% idle).
- Correção recomendada (baixo risco): loop `invlpg` por página + barreira em
  `vm_mprotect` após `pml4_walk_ptes` (ver Fase 5).

### A2. [CRÍTICO] `signal_dispatch_irq` usa índices errados do frame do irq0
- Arquivo: `OvsbMk/kernel/signal.c:248-286`.
- Gate: `registers[18] & 3` — no frame do irq0 (`idt.asm`, 15 GPRs + 5 do CPU),
  CS está em `registers[16]` e `registers[18]` é o **RSP** (16-alinhado ⇒ `&3==0`
  quase sempre ⇒ entrega via IRQ quase nunca acontece; quando acontece por
  azar, é errado).
- Mapeamento: `adapted[15] = registers[17]` pega **RFLAGS** como RIP, e
  `adapted[16] = registers[19]` pega **SS (0x23)** como RFLAGS. O correto seria
  `adapted[15] = registers[15]` (RIP) e `adapted[16] = registers[17]` (RFLAGS).
- Efeito: sinal entregue via timer planta RIP=RFLAGS / RFLAGS=0x23 ⇒ crash
  determinístico quando o gate falha-aberto. Raro (gate quase sempre fecha),
  mas é corrupção de retorno real. Documentado como recomendação (Fase 5).

### A3. [SUSPEITO] layouts de frame divergentes entre portas de entrada
- `syscall_entry.asm`: regs[0]=rax … regs[14]=r15, regs[15]=RIP, regs[16]=RFLAGS.
- `syscall_isr` (int 0x80): mesmo layout da syscall_entry. OK.
- `irq0`/`isr_common`: regs[0]=r15 … regs[14]=rax (+int_no/err em isr_common).
- `context_switch` (switch.asm) faz pop na ordem da syscall_entry; processo
  salvo via irq0 e restaurado por esse caminho teria GPRs permutados (o iretq
  ainda acerta porque são 15 qwords de qualquer forma). `signal.c` já compensa
  com a inversão `adapted[i]=registers[14-i]` — evidência de que a divergência
  é conhecida e tratada pontualmente. Recomendação: unificar ou documentar
  (Fase 5, `./docs/`).

### A4. Ponto cego: as 6 instruções entre o epílogo C e o `iretq`
Desmontado de `OvsbMk/kernel/syscall_entry.asm` (offsets no `.text`):
```
63: 48 89 4c 24 d8        mov [rsp-0x28],rcx        ; (1) RIP: return original (rcx pós-syscall)
68: 48 c7 44 24 e0 1b 00  mov [rsp-0x20],0x1b        ; (2) CS = 0x1B user-code ring3
00 00
71: 4c 89 5c 24 e8        mov [rsp-0x18],r11        ; (3) RFLAGS: return original
76: 48 89 64 24 f0        mov [rsp-0x10],rsp        ; (4) RSP = user RSP original
7b: 48 c7 44 24 f8 23 00  mov [rsp-0x8],0x23         ; (5) SS = 0x23 user-data ring3
00 00
84: 48 83 ec 28           sub rsp,0x28              ; (6) publica o frame (40 bytes)
88: 48 cf                 iretq                     ; consome RIP/CS/RFLAGS/RSP/SS
```
- Ordem de escrita correta; `sub` por último "publica" o frame de uma vez.
- Janela de race (1)-(6): escritas ABAIXO do rsp (red-zone do usuário). IRQ/NMI
  chaveiam pra kernel-stack via TSS, então não clobberam; o risco real seria
  outro thread do mesmo `mm` com stack sobreposta — improvável com o layout
  atual. Sem `cli` necessário (FMASK já limpa IF na entrada `syscall`).
- Sem validação de que o user-RSP é canônico/user antes das escritas: um RSP
  selvagem (ex.: thread com stack corrompida — cf. `RBP=0x2C0` no crash) faz as
  5 escritas falharem em CPL0 ⇒ `#PF` em ring 0 ⇒ pânico/hang em vez de
  `PF-KILL` limpo. Recomendação: checar canonicidade + bit U antes de
  escrever, ou reordenar para `sub` primeiro (frame sempre "dentro" da stack).
- Correlação nova: os 6 bytes `OP= 48 C7 44 24 E0 1B` dumpados no crash
  (`idt.c:247-252` lê 6 bytes no RIP usuário) são BYTE-IDÊNTICOS à instrução
  (2) acima (`mov [rsp-0x20],0x1b`). O userland OvsbOS usa `int 0x80` (sem stub
  iretq), logo nenhum binário legítimo contém esse padrão — o RIP 0x08027478
  é endereço desviado (stack smash / frame stale) OU coincidência de bytes de
  dados decodificados como código após salto selvagem. Ver `hipotese-final.md`.

### A5. Leituras de memória user sem validação no caminho de sinal/PF
- `signal.c:225,302,306-307`: `*(volatile uint64_t*)(user_rsp+0x88)` e
  `(fs_base+0x28)` sem probe — RSP selvagem ⇒ PF em ring 0 dentro do handler.
- `idt.c:248-252`: dump `OP=` lê 6 bytes no RIP usuário sem probe (se o RIP for
  não-mapeado, PF aninhado em ring 0). Baixo risco prático (IDL mapeia baixo?),
  mas anotado.
