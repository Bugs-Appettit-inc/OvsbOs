# FASE 3 — VirGL 2D consolidado

## 4 fixes anteriores: VALIDADOS
- Formato inválido (tela preta) → corrigido (format=1); tela visível.
- Descritores sem reciclagem → corrigido (free-list + máscara `% size`);
  flushes continuam após 1000+ comandos (era ~64).
- Arg4 no trampolim → corrigido (`push r10`); `mini: r=0 ...` prova.
- PDE 2MB mal-alinhada → corrigido (buffers 2MB); `drmtest` PASS.

## Benchmark (10s, fullscreen 1280x800 + flush/frame)
- VESA: ~160 fps média (fill 4MB em CPU, host lê RAM direto).
- **virtio-gpu: ~9 fps média** (26,19,29,1,4,2,4). Gargalo: upload GL
  de 4MB/frame no nouveau (NVD9) + roundtrip de flush por frame.
- Conclusão: fullscreen a cada frame é patológico; updates parciais
  (`disp_flush_rect`) + cursor fast-path são o caminho (já implementados).

## Stress (esta noite)
Cliques HMP + rajadas de motion + scans: 0 hangs do QEMU atribuíveis
ao guest; faults só o `+0x68` conhecido (scan-child, compositor, apps).
QEMU morreu 3x sem stderr (host-side: OOM/harness; sessões SDL longas
íntegras). `cache=none` no drive reduziu pressão de page-cache.
