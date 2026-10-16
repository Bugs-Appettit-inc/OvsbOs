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
