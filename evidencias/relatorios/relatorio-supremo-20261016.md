# Relatório supremo — 2026-10-16 (branch `suprema/20261015-2140`)

## 1. Resumo executivo

A noite suprema virou dois turnos: no primeiro (fases 0–6) validei o fix
`invlpg` com QEMU headless (0 faults), consolidei os 4 apps, criei a lib de
imagens (BMP+PPM+PNG com inflate próprio), o empacotador `.oep`, a auditoria
do kernel e as dev-tools. No segundo turno (este, por ordem direta: sem
stress de 2h, só limpa) revisei **todos os arquivos linha a linha**,
achei e corrigi **11 bugs reais** (2 com corrupção de dados), recuperei o
`shell/main.c` de uma corrupção por disco cheio, e deixei tudo compilando
limpo (`-Wall -Wextra -Werror`) com testes de fumaça passando. CHIP-8
implementado e verificado (14 px, 2 ROMs próprias). Game Boy, JPEG, stress
2h e merge em `main` ficam para depois (motivos no §12).

## 2. FASE 1: fix `invlpg` (turno 1)

- `make -C OvsbMk clean && make -C OvsbMk`: OK; `md5sum`
  `build/kernel.elf` == `iso/boot/kernel.elf` (`d5851dd3…`, depois
  `efe86d91…` após melhoria `env.c`); ISO regenerada com `grub-mkrescue`.
- Headless QEMU (`-display none`, serial em `/tmp/sup-headless.log`):
  bootou, compositor Wayland subiu (wallpaper/bar commits no log),
  **0 ocorrências de `PF-KILL|EXC#|PF@|SLAB-BAD`** no período observado.
- Reprodutor 20 cliques / stress guest: **não executado** (sem sessão
  QEMU com display nesta noite).
- **CONCLUSÃO: fix plausível e sem regressão no boot, mas NÃO validado
  contra o reprodutor — merge em `main` NÃO recomendado ainda.**

## 3. FASE 2: apps — status

| App | LOC | Build | Roda | Fecha | Screenshot |
|---|---|---|---|---|---|
| editor | ~552 | limpo | sim | sim, sem zumbi | `evidencias/suprema/apps/editor.ppm` |
| paint | ~336 | limpo | sim | sim | `.../paint.ppm` (pixels reais) |
| fm | ~374 | limpo | sim | sim | `.../fm.ppm` |
| shell | ~401 | limpo | sim | sim | `.../shell.ppm` |

Memória (pico RSS host, medido na noite anterior): editor 252 kB,
paint 244 kB, fm 1508 kB, shell 252 kB.
Instalados no `disk.img`: `/BIN/EDITOR /BIN/PAINT /BIN/FM /BIN/OVSH`
(menu auto-escaneia). Zumbis: 0 (o único `defunct` do sistema era
`colord-sane`, pré-existente, sem relação).

## 4. FASE 3: imagens

- `userspace/libs/image/`: BMP 24/32-bit, PPM P6, **PNG com inflate
  próprio** (stored/fixed/dynamic Huffman, filtros 0–4, gray/RGB/RGBA),
  `img_scale/crop/blend/ascii`, Makefile estático, sem dependências,
  `-mno-sse -mno-mmx -mgeneral-regs-only`. `imgtest`: **RESULT: PASS**.
- JPEG: **bloqueado** (baseline DCT ≈ 1000+ linhas, fora do budget; PNG
  consumiu o budget de bônus e passou).
- Integração: paint lê/escreve ppm/bmp/png; editor `--dump-img`;
  fm tecla `i` (preview ASCII). Roundtrips verificados (bmp→ppm idêntico,
  PNGs reais decodificam com pixels corretos).

## 5. FASE 4: empacotador `.oep`/`.odk`

`userspace/tools/oep-pack/`: `oep-pack/list/unpack/run` (mesmo fonte,
multi-call por `argv[0]`). Formato v1 (`OEP\0`, manifesto JSON, seções
ELF/asset/ícone). Verificado: pack→list→unpack binário **idêntico**
(`cmp`), `oep-run` executa igual ao binário cru. 4 apps empacotados em
`evidencias/suprema/oep/`.

## 6. FASE 5: auditoria kernel

40 arquivos em `OvsbMk/kernel/`: OK=10, SUSPEITO=29, CRÍTICO(auto)=1
(`env.c`, corrigido: `strcpy`→`memcpy` exato + declaração `memcpy` em
`utils.h`). Trampolim redesmontado (bytes idênticos). Recomendadas e
**não aplicadas** (arriscadas): índices do `signal_dispatch_irq`,
validação de user-RSP, probes sem validação (`melhorias-recomendadas.md`).

## 7. FASE 6: dev inside

Editor: highlight C/ASM/sh (`--hl=`), busca (Ctrl+F), **substituir
(Ctrl+R)**, números de linha (`--ln`/Ctrl+N). `oasm` (mov/add/jmp,
`jmp` relativo verificado: `E9 F1 FF FF FF`), `omake` (rebuild por
mtime verificado), `oman` (lista/mostra/busca em `docs/`). TCC/NASM no
guest: bloqueado (porte fora do escopo; `oasm` cumpre o item
mini-assembler).

## 8. FASE 7: emulador

- **CHIP-8: PASS.** `userspace/apps/chip8/` (~322 linhas, todos os
  opcodes, wrap de sprite, timers), TUI + `--gui` fb + `--cycles/--dump`
  p/ teste. 2 ROMs próprias (domínio público por autoria):
  `test_draw.ch8`, `test_bounce.ch8` — 200 ciclos sem halt, 14 px corretos.
- Game Boy: **não iniciado** (médio, ~2000 linhas, sem budget após a
  limpa). NES/DOSBox: inviáveis numa noite.
- **GTA 1: objetivo de longo prazo.** Motivo: engine 3D proprietária
  (PS1/DOS), sem código-fonte, exigiria reimplementação de renderizador
  + física + áudio + streaming de cidade — anos, não horas. Caminho
  realista: CHIP-8 → Game Boy → NES → PS1-HLE, um por ciclo.

## 9. FASE 8: stress

**Não executado por ordem direta** (sem 2h). No lugar: bateria de
fumaça em todos os binários + `imgtest` + roundtrip oep, tudo PASS.
`evidencias/stress.sh` (noite anterior, 20 iters host, 0 falhas)
continua válido e pronto para a sessão longa.

## 10. A limpa (este turno): 11 bugs reais

| # | Arquivo | Bug | Severidade | Status |
|---|---|---|---|---|
| 1 | editor `ed_enter` | `memmove` destino `g_buf[g_nlines]` embaralhava linhas | **corrupção** | corrigido + testado (3 casos) |
| 2 | editor `ed_enter` | `memmove` de lens não preservava length da última linha (linhas "sumiam") | **corrupção** | corrigido + testado |
| 3 | paint undo | `g_uw/g_uh` únicos → undo restaurava dimensão errada após `new`/`load` | dados errados | corrigido (por slot) + testado |
| 4 | paint `pfill` | pilha 4× menor que pior caso → fill parcial em canvas grande | incompleto | pilha full-size + testado |
| 5 | paint GUI | borracha não apagava nada (press e drag sem efeito) | inútil | corrigido |
| 6 | paint `new` | snapshot DEPOIS de limpar → undo inútil | UX | ordem corrigida + testado |
| 7 | fm preview | `while(getchar()!='\n')` = loop infinito em EOF | hang | corrigido |
| 8 | fm `open` | command injection via `'` no nome do arquivo | **segurança** | sanitize + testado |
| 9 | fm paste | colar sobre si mesmo zerava o arquivo (`O_TRUNC` no próprio src) | **perda de dados** | guarda origem=destino + testado |
| 10 | fm GUI | `ln[cols-1]` com `cols>256` = OOB write; `cols=0` = wrap | crash/OOB | cap + guarda |
| 11 | png.c | leak `prev/cur` em filtro inválido; CMF só-0x78; sem teto p/ IDAT-bomb | leak/robustez | corrigido |

Melhorias menores: TAB no editor (4 espaços), `u` volta no fm (Backspace
não chega em stdin canônico), `echo x > f` interno no shell (sem
`/bin/echo` no guest), `fflush` antes de `fork`, `ls` com nome limitado
por reclen, guards de dims/px nulos na lib, `pitch` validado no fb,
`1<<Vx` com Vx≥16 (UB) no CHIP-8, `usleep` no loop GUI, `nsec` no ODK,
atalho `.` escondido no fm, `main` duplicado removido (ver §11).

Deixado como está (sem bug): scancode-map (conferido índice a índice),
Bresenham, elipse, `tokenize` (já corrigido na noite anterior),
inflate dinâmico (tabelas RFC conferidas), layouts de frame do kernel
(fora do escopo desta limpa), `fm_up`, waitpid/EINTR, `oasm` rel-jmp.

## 11. Incidentes de infra

- **Disco 100% cheio** (sistema, 99G/104G — não é do repo): links
  falharam 3×; contornei compilando para `/tmp` (tmpfs, 3.9G livres).
- **Corrupção do `shell/main.c`** por escrita parcial com disco cheio
  (`main` duplicado + linhas fundidas). Recuperado por splice + revisão
  completa; faltou 1 fix (reaplicado e verificado). Todos os 11 fontes
  passaram em `gcc -fsyntax-only` + build `-Werror` limpo após o episódio.
- `pkill -f qemu` casa com a própria linha de comando (suicídio do shell
  na noite anterior) — usar script + `setsid`.
- PNGs de screenshot da noite anterior estavam malformados (fatia sem
  `*3` no gerador) — o decoder estava certo em rejeitar; geradores
  corrigidos e PNGs regenerados.

## 12. O que NÃO foi feito e por quê

1. Stress 2h: **ordem direta** (fazer depois, `stress.sh` pronto).
2. Reprodutor 20 cliques: sem QEMU com display nesta sessão.
3. Game Boy / JPEG / TCC-no-guest: sem budget (prioridade foi a limpa).
4. Merge em `main`: **não** — fix `invlpg` segue não-validado no
   reprodutor; branch `suprema/20261015-2140` pronta p/ revisão.
5. `PROMPT-NOITE.md` (modificado antes desta noite): não tocado.

Commits do turno: `9d4ff25` (limpa + chip8). `git log` em
`evidencias/` (atualizar antes do push matinal).
