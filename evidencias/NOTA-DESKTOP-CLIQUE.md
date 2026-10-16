# Nota: clique no Iniciar (wallpaper sem barra/mouse + flick)

## Diagnóstico (com evidência)
- pid 9 do crash +0x68 **era o próprio `/BIN/OVSB-DESKTOP`**: o clique
  chamava `menu_try_open` → `scan_dir("/BIN")+scan_dir("/APPS")`
  (~100 open/read/close) e morria no meio → sem menu; superfícies
  wallpaper+barra ficavam stale/mortas conforme o compositor purgava.
- Mesa **fora do caminho**: XORG/FVWM estão comentados no boot; o desktop
  Wayland faz blits de CPU direto no fb mmapado. Mexer no mesa não afeta
  estes sintomas.
- Flicker no mouse: `redraw_all()` no compositor = fill 1280x800 + reblit
  de tudo + cursor **a cada motion** (direto no fb visível, sem vsync) =
  tearing garantido. Desktop também commitava a barra a cada wake do poll.

## Feito (sem tocar no compositor)
- `ovsb-desktop.c`: cache do scan (mtime de /BIN//APPS; 2 stats vs ~100
  open/reads por clique) + commit da barra só quando o segundo muda.
- Rebuild + reinstall `/BIN/OVSB-DESKTOP`; headless OK (wallpaper+bar,
  clock, 0 faults).

## Pendente de aprovação (regra: compositor não se toca sem aval)
- `evidencias/proposta-flicker-compositor.diff`: fast-path do cursor
  (salva/restaura caixa 12x16; invalida em qualquer blit/drag/resize;
  repaint total só em evento estrutural). ~165px vs ~1M px por motion.

## Validado em 2026-10-16 (QEMU + HMP inject, 3 cliques reais)
- `menu overlay criado 224x648 apps=40` em 3/3 cliques, fecha com
  clique-fora, **0 PF-KILL** no log inteiro.
- Fork-isolation: filho morreu 1x no teste anterior e o desktop seguiu
  (menu abriu). Não zera mais a lista em falha (temp + commit).
- Pre-warm no boot + `menu_ensure()` (valida) + retry implícito.
- Desktop com retry-infinito noועים connect (race do boot resolvida).
- memfd fechado após `send_msg_fd` (vazamento por abertura eliminado).
- Instrumentação USTK no kernel (read-only, com guarda de page-walk):
  frame iretq íntegro exceto slot RIP; stack do usuário saudável =
  corrupção só no RIP de retorno (kernel segue investigando; DIAG).
- Experimento poison revertido (falso-positivo por preempção) + append
  no PCB revertido (travava boot early — .bss maior; causa exata do
  hang em aberto, anotado p/ revisão).
