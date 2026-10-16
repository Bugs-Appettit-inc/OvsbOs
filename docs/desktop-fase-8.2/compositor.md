# Compositor hand-rolled — globais, input, WM, roteamento

Arquivo: `OvsbMk/tests/compositor/wayland_compositor_lib.c` (~1600 linhas).
**REGRA: não tocar sem autorização expressa.**

## Globais principais
- `fb_addr/fb_w/fb_h/fb_pitch/fb_fd`: framebuffer (`/dev/fb0` mmapado).
- `surfs` (lista fundo→frente): `struct surf` = janela/layer com
  `wx,wy,ww,wh,mapped,buffer,is_layer,layer` (+ xdg, subsurface, foco).
- `cursor_x/cursor_y` (origem 120,80), `cursor_dirty/cursor_idle_on`
  (coalesce de motion), `cursor_under[12x16]` + `cursor_saved/stale`
  (fast-path: salva/restaura sob o cursor em vez de repaint total).
- `drag_surf/drag_off`, `rs` (resize), `focused_surf`, `ptr_focus`.

## Input
- evdev `/dev/input/event0` (mouse) + `event1` (teclado) via event loop;
  `evdev_fd_cb` decodifica `EV_REL` (cursor) e `EV_KEY` (botões/teclas).
- Teclado vai só para janelas xdg com foco; **layers não recebem kbd**.

## WM e roteamento de pointer
- `wm_pick_layer(x,y)`: layer top/overlay primeiro (sem Alt); com Alt,
  cai no drag xdg. Clique em layer consome (xdg embaixo não vê).
- `wm_pick()`: janela xdg de cima para baixo; Alt+botão = move,
  borda = resize (throttle 8px); clique foca.
- Enter/motion/button + frame são enviados ao cliente dono; layers
  recebem enter/motion/button (sem roubo de foco de teclado).

## Repaint
- `redraw_all()`: fill cinza + layers 0/1 + xdg + layers 2/3 + cursor.
  Disparado por commits e (coalescido) por motion.
- `fb_blit_shm()`: blit ARGB8888 com clip + `draw_cursor()` por cima;
  logs de blit sob `WL_LOG_HOT=1` (padrão: silencioso).
- `wm_blit_layer()` / `wm_blit_win()` (titlebar + conteúdo 24px abaixo).
