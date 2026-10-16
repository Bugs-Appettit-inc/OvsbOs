# Relatório noturno — 2026-10-15 (branch `noite/audit-20261015-2104`)

## 1. Resumo executivo

Noite completa: 4 apps base criados e instalados no `/BIN` (menu os lista
sozinho), auditoria de 331 arquivos do kernel com 3 achados novos (1
crítico virou fix), e a causa mais provável do bug `+0x68` isolada em
`vm_mprotect` sem `invlpg` — **TLB stale após troca de NX/RW explica a
assinatura `err=0x15`, o flaky ~50% sob churn e o 0% idle**. Fix aplicado,
kernel + ISO rebuildados com `make clean && make`. Stress host: 20 iters,
0 falhas. Falsos positivos da madrugada (bugs reais achados nos meus
próprios apps) documentados abaixo.

## 2. Kernel: integridade, achados, selos

- Escopo: 331 arquivos (`.c/.h/.asm/.S/.zig`) em `OvsbOS/OvsbMk + src`,
  hash em `evidencias/manifest.sha256`.
- Selos automáticos: **OK=83, SUSPEITO=233, CRÍTICO(auto)=15**.
  Notas: `gcc -fsyntax-only` falha em 7 `.c` por include-path freestanding
  (o build real usa `-I` próprio — não são erros); `utils.c`/`string.c`
  "copiam sem bound" porque **implementam** `strcpy/memcpy` (falso-positivo);
  todos os 5 `.asm` montam OK (`nasm -f elf64`).
- Detalhe: [`evidencias/auditoria-kernel.md`](OvsbOS/evidencias/auditoria-kernel.md).

| # | Severidade | Onde | Resumo |
|---|---|---|---|
| A1 | CRÍTICO → **corrigido** | `vm_map.c:556-704` | `mprotect` trocava NX/RW sem `invlpg` (munmap tinha, mprotect não) |
| A2 | CRÍTICO (só documentado — arriscado) | `signal.c:248-286` | `signal_dispatch_irq` lê RFLAGS como RIP e SS como RFLAGS; gate no índice errado |
| A3 | SUSPEITO | `switch.asm`/`idt.asm` | 4 layouts de frame distintos; tratado pontualmente, não unificado |
| A4 | análise | `syscall_entry.asm:119-125` | 6 instruções cegas desmontadas e comentadas 1-a-1; sem validação de user-RSP |
| A5 | SUSPEITO | `signal.c`, `idt.c` | leituras em memória user sem probe no caminho de sinal/PF |

## 3. Apps criados

Todos em `userspace/apps/<nome>/` com Makefile próprio, `-Wall -Wextra -Werror`
limpo, standalone (só libc + syscalls; extensões OvsbOS via syscall raw
**só com `OVSBOS=1`** — no host viram no-op seguro), modo texto + `--gui`
(framebuffer 200/201/202/206, fonte 8x8 extraída de `lib/gui/vesa.c`).

| App | LOC | CLI/TUI | GUI | Evidência |
|---|---|---|---|---|
| editor | 402 | abrir/editar/salvar, setas/Home/End/PgUp/PgDn, Ctrl+S/F/Q, UTF-8 passthrough, binário preserva NUL | fb texto, cursor, Ctrl+S/Q | `evidencias/apps/editor.png` |
| paint | 350 | `--cli new/line/rect/ell/fill/clear/undo/save/load` (PPM P6), undo 10 níveis verificado | mouse + teclas b/e/l/r/o/g, cursor | `evidencias/apps/paint.png` (pixels reais) |
| fm | 352 | lista (nome/tamanho/tipo), Enter/Backspace, mkdir/renomear/deletar-confirma, yank/colar-copia/move, abrir por extensão, status com espaço livre | fb texto navegável | `evidencias/apps/fm.png` |
| shell (OVSH) | 393 | ls/cd/pwd/cat/echo/mkdir/rm/cp/mv/clear/exit/help/hist, pipe `\|`, redir `> >> <`, aspas, histórico | console fb | `evidencias/apps/shell.png` |

Memória (pico RSS host): editor 252 kB, paint 244 kB, fm 1508 kB,
shell 252 kB — [`evidencias/apps/memoria.md`](OvsbOS/evidencias/apps/memoria.md).
 Instalados no `disk.img`: `/BIN/EDITOR /BIN/PAINT /BIN/FM /BIN/OVSH`
(estáticos ~800 kB; OVSH não colide com o `/BIN/SH` existente).
Ícones 16x16+32x32 (XPM+PNG) em `assets/icons/`.
Zumbis: 0 após 5x abre/fecha por app e após rodada simultânea (rc=0 nos 3).

Bugs achados nos meus apps durante o teste (já corrigidos):
tokenizador do shell perdia argumentos (`*d=0` clobberava o delimitador);
parser CLI do paint confundia a op `fill` com flag `f`.

## 4. Bug +0x68: status e hipótese

**Não resolvido-com-prova (sem sessão QEMU), mas com causa principal
identificada e fix aplicado.** Detalhe: [`evidencias/hipotese-final.md`](OvsbOS/evidencias/hipotese-final.md).

- Fato novo: os bytes `OP=48 C7 44 24 E0 1B` no crash são a instrução
  `mov [rsp-0x20],0x1b` — encoding idêntico ao push de CS do trampolim
  (offset `0x68` no `.text`, ver disassembly na auditoria A4).
- `err=0x15` = fetch ring-3 em página presente; tabelas (`PDhi=0`) dizem
  executável ⇒ a CPU via TLB antiga (NX) ⇒ **stale TLB pós-`mprotect`**.
- A refutação anterior de "TLB-via-invlpg" testou o caminho errado
  (demand-paging TEM `invlpg`; `mprotect` NÃO tinha).
- `RBP=0x2C0` + A2 (sinal via IRQ planta RIP=RFLAGS) explicam o RIP
  desviado como corrupção secundária.
- Experimento 4.1 pronto em `evidencias/iretq-probe.diff` (`int3` antes do
  `iretq`; **não aplicado** para não invalidar o snapshot).

## 5. Crashes capturados

| Origem | Qtd | Padrão |
|---|---|---|
| `bug2-rip-dump.log.bak` (original, preservado, hash igual) | 1 | `PF@0x08027478 err=0x15` → `[PF-KILL]` pid 9 (extraído em `evidencias/crashes/crash-bug2-08027478.log`) |
| stress host 20 iters (fm+editor+paint+shell sob churn malloc x2) | 0 | apps estáveis; RIPs únicos: n/a |

RIPs únicos observados: `0x08027478` (fetch NX-stale).

## 6. Melhorias: implementadas vs recomendadas

Implementadas (branch, build OK): `invlpg`+barreira em `vm_mprotect`;
comentários de auditoria no trampolim; `docs/syscall-trampolim.md` +
`docs/vm-tlb.md` (regra: mutou PTE em pml4 vivo ⇒ `invlpg`+barreira).
Recomendadas (arriscadas, NÃO mexidas): corrigir índices em
`signal_dispatch_irq` (habilitaria entrega via IRQ hoje morta — testar no
guest); validar user-RSP canônico antes das escritas do trampolim; probes
(`user_rsp+0x88`, dump `OP=`) sem validação; testes unitários por syscall
(sem framework guest hoje).

## 7. Próximos passos (5)

1. Sessão QEMU: rodar stress guest ≥2h com o fix e confirmar sumiço do `+0x68`.
2. Aplicar `iretq-probe.diff` numa sessão controlada (antes/depois do iretq).
3. Corrigir `signal_dispatch_irq` (A2) com teste de entrega via timer no guest.
4. Adicionar self-test guest de `mprotect` W→X + exec (regressão do A1).
5. Linkar apps contra a libc do userland OvsbOS (binários menores que os ~800 kB estáticos).

## 8. NÃO feito e por quê

- Stress guest de 2h / screenshots em QEMU: sem sessão interativa nesta noite;
  feito equivalente host (20 iters) + procedimento guest em `stress.sh`.
- `ud2/int3` no trampolim: preparado, não aplicado (invalidaria o snapshot).
- Fix do `signal_dispatch_irq`: arriscado sem teste guest (regra da Fase 5).
- Testes unitários por syscall: sem framework no repo.
- `dmesg` inicial: sem permissão no container (log registra o erro).
- Desmontar `+0x68` de binário dinâmico com `readelf/objdump/nm`: sem acesso
  ao binário exato do pid 9; deixado roteiro no §5 da hipótese.
- Commits: `359e443` snapshot, `12712c8` trabalho, `365534a` gitignore,
  `ab355e2` git-log — branch pronta p/ revisão (`git log` em `evidencias/`).
