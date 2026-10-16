# Desktop OvsbOS — Fase 8.2 (congelado pré-VirGL)

Cliente Wayland hand-rolled (`OvsbMk/tests/desktop/ovsb-desktop.c`, ~660
linhas, C puro, protocolo raw, sem libwayland) + compositor hand-rolled
(`OvsbMk/tests/compositor/wayland_compositor_lib.c`).

O que faz: wallpaper em gradiente (1280x800) + barra superior 24px com
botão Iniciar e relógio (CLOCK_REALTIME do kernel) + menu Iniciar em
overlay (layer_shell, 224px, ancorado TOP|LEFT com margem de 24px) que
lista ELFs de `/BIN` e `/APPS` + launcher (duplo fork + execve).

Boot: kernel (`shell_init`) executa `/BIN/WAYLAND_COMPOSITOR_LIB &` e
`/BIN/OVSB-DESKTOP &`. O desktop conecta em `/tmp/wayland-0` (retry
infinito), faz registry, cria wallpaper (layer 0) e barra (layer 2,
exclusive 24), entra no loop (ack configures + relógio + pointer).

Robustez (histórico de guerra, ver `bugs-conhecidos.md`): scan do menu
isolado em processo filho (o clique NUNCA mata o desktop), cache por
mtime, pre-warm no boot, commit da barra só quando o segundo muda,
memfd fechado após envio, auto-cura com relançamento do compositor
(máx. 3 sessões, menu sobrevive). Testado com cliques reais via HMP:
menu abre 3/3 com 40 apps.

Ver: `compositor.md`, `apps.md`, `bugs-conhecidos.md`,
`proximos-passos.md`, `estrutura.md`.
