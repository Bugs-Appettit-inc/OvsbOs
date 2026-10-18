# FASE 0 — Log do crash (diag-visual, 2026-10-16)

Log: /tmp/boot-gl.log (43906 bytes, mtime 21:33:39) — copiado p/ evidencias/diag-visual/boot-gl.log
Boot: boot .oep (PTY_TEST…HELLO_DYN, EDITOR.OEP), compositor=PI0x09, desktop=PI0x0A (OVSB-DESK.OEP)

Sequência final (linhas 741-830):
1. desktop PID 10: wallpaper+barra commitados, menu pre-warm (menu_ensure) -> fork filho 0x0B
2. filho 0x0B (scan /BIN, open/read/close por arquivo) morre:
   PF@ RIP=0x00000008 err=0x15 (user, instr fetch) -> [PF-KILL] pid=0x0B
3. desktop: [DESKTOP] menu rescan apps=0 (filho morreu no meio do scan)
4. compositor PID 9 morre em seguida:
   PF@ RIP=0x08028239 err=0x15, PD[0x40]=0x080000E3 -> [PF-KILL] pid=0x09
5. desktop segue 1x (write socket peer=2, n=8) e o LOG TERMINA — sem PANIC/EXC#/thexit.

- Último PID/syscall: PID 0x0A (desktop), write() fd=3 sock → compositor (já morto). Nada mais.
- Assinatura: mesma família do +0x68 (RIP desviado p/ 0x080xxxxx / 0x8, err=0x15);
  difere do crash clássico (0x08027478) só no valor do RIP.
- Morte do QEMU sem saída do kernel = consistente com triple fault (KVM reset + -no-reboot => exit)
  OU crash do host (SDL/GL). Distinguir via stderr do QEMU + teste headless.
