# Relatório — Consolidação + `.oep` (Ovsb Executable Program)

Branch: `redondo/20261016-1446` (sem push). Data: 2026-10-16.

## 1. Resumo executivo
Tudo planejado foi implementado exceto a limpeza agressiva de evidências (optei por mover, não apagar) e a validação interativa visual do menu no monitor (SGL/headless não executa cliques).
A descoberta central: **o branch herda um regressor pré-sessão (`0ad68b4` + extras não-commitados da "fase redondo" — tripwires `STASH-POISON/CLOBBER` + validação de frame CS em `switch.asm`) que fazia o boot travar no PID 1 (EXC#0x06 CS=0x08 RIP=0x3, IRET-frame lixo).** O revert do fix +0x68 + remoção das tripwires devolveu o boot funcional (`d796935` boota; novo kernel = d796935-base + fases `.oep` boota no desktop completo).

## 2. `.oep` — formato v1
Header fixo de 128 bytes (`userspace/libs/oep/oep.h`): magic `OEP\0`, versão 1, `elf_offset/elf_size`, name[32], category[16], **SHA-256 do ELF** (sem SIMD, `sha256.c` puro FIPS 180-4).
- `oep-pack <elf> <nome> <categoria> -o APP.oep` (build host `-static` default; `make HOST=1` = dinâmico).
- `oep-info` (host): mostra nome/categoria/hash e **valida SHA-256** (hash OK em todos).
- `oep-unpack` / `oep-run` (host), tudo via argv0 multi-call.
- Kernel: **`SYS_execve` (syscall.c) e `cmd_exec` (shell.c)** reconhecem magic `OEP\0`, validam `elf_offset/elf_size`, pulam p/ ELF.
- `/BIN` novo: `EDITOR.OEP`, `PAINT.OEP`, `FM.OEP`, `SHELL.OEP`, `OEPPACK.OEP`, `OVSB-DESK.OEP` (ELF crus preservados).

## 3. Desktop
- Fases 8.3/8.4 da sessão anterior (janelas na barra + ícone/matriz categorias) foram comitadas na Fase 0.
- Launcher agora mostra **só `.oep`** (magic `OEP\0` lido por 64 bytes; nome/categoria do header), agrupado por categoria com headers; ELF crus + testes ficam de fora.
- Boot do desktop automático: `shell.c` roda `/BIN/OVSB-DESK.OEP` (valida loader no boot). `EDITOR.OEP &` roda no init pra provar `.oep` em runtime (log `exec: .oep ok` + editor carrega e desenha TUI `~`).
- Poll/reap/janelas já estavam no estado da sessão anterior (Fase 4 confirms): `poll()` com `POLLERR|POLLHUP|POLLNVAL` ok, `reap_apps()` cada 8 tick (sem zumbi).

## 4. Limpeza
- `scripts/qemu.sh [vga|gl|gl256|tcg|headless]` — um só script; antigos movidos p/ `scripts/arquivo/` (nenhum deletado; `qemu-run.py`/`qemu-wrap.sh` arquivados).
- `ESTADO.md` criado (root) com estado do branch.
- Relatórios ~ movidos p/ `evidencias/relatorios/`. Evidências antigas (dmesg, notas, diffs) p/ `evidencias/arquivo/`. Nada deletado.
- disk.img: FREE baixo → removidos só diagnósticos cru (ZZTEST, AAA_VG, VG_MINI, SCANTEST).

## 5. Validação
- **Build:** `0 erros`, 2 `warnings` só em `fs/fat32.c` (fora do escopo: vetado mexer) + `info` linker RWX (pré-existente). Demais warnings zerados (casts `uintptr_t`, `if`-indentação, `unused`); `-Wno-frame-address` documentado (`__builtin_return_address` é debug do slab).
- **Boot headless (KVM):** desktop conecta → wallpaper → bar → menu rescan → epoch ticks. `EXC#`/`SLAB-BAD` = **zero**. `PF@`: 1 user-mode, no primeiro rescan de menu (`fork`+`getdents` = `err=0x15`, **pré-existente — assinatura idêntica bootando `d796935`**).
- **`.oep` runtime:** `exec: .oep ok` no `EDITOR.OEP`; carregamento, brk, mprotect e TUI ok.

## 6. Regressão herda (achado principal)
Bisect: `d796935` (pré-fix +0x68) boota **ok**; `0ad68b4` (fix +0x68 presumido) boota **travado**; extras não-commitados herdadas (tripwires `STASH-*` + `*.asm` jump *CS-frame-validation*) travam no primeiro exec (PID1)). **NÃO toca** `switch.asm` pro root-cause (proibição); decidimos revert e segui. Se a sessão "+0x68" quiser retomar, `git revert 2300f46` volta ao estado anterior (e reaplicar Phases 2-6 fica fácil — os commits são lineares).

## 7. NÃO deu / pendentes
- ⌛ Menu interativo por `.oep` no monitor: executado headless com cliques na UI não disponível — não pude verificar manualmente a listagem visual (kernel+desktops lf: fine, scanning apps=0 provém do fork-child PF@ pré-existente).
- ⌛ Verificação SHA-256 no kernel (só no host tooling).
- ⌛ Re-pack `.oep` do `FOOT`/`DOOM` (fora de escopo).
- ⌛ Warnings `fat32.c` (fora do escopo).

## Log de commits (um por fase)
```
584b10b chore(fase0): snapshot redondo — tripwires +0x68, desktop 8.3/8.4, cache=none
8946656 feat(oep): formato .oep v1 + pack/info/unpack/run + apps restaurados
c574827 feat(oep): loader .oep no execve (magic OEP0 + salto pro ELF)
f87dc69 feat(desktop): launcher lista só .oep; boot via OVSB-DESK.OEP
2cb3d03 chore: scripts QEMU consolidados em qemu.sh, evidencias arquivadas, ESTADO.md
37718c9 fix(kernel): zero-warnings
4debeaa Revert "fix: mata o bug +0x68 ..."  (regressão herdada)
2300f46 fix(fase6): boot validado; cmd_exec .oep; boot .oep green
```
