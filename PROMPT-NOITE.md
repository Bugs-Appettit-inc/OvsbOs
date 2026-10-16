# TAREFA NOTURNA SUPREMA — OvsbOS: Desktop + Apps + Imagens + Empacotador + Dev Inside + Emulador + Auditoria + Stress

**Você é um engenheiro de sistemas sênior, trabalhando sozinho a noite toda.**
**Execute TUDO autonomamente. Se travar numa fase, documente o bloqueio e SIGA.**
**Ao final, gere UM relatório único em `~/relatorio-supremo-$(date +%Y%m%d).md`.**

---

## CONTEXTO DO PROJETO

- **Kernel:** OvsbMk (C + Zig + ASM), x86_64, compat Linux ELF.
- **Stack gráfica:** Wayland hand-rolled (`wayland_compositor_lib.c`, NÃO tocar sem autorização).
- **Desktop:** `ovsb-desktop.c` (wallpaper + barra + relógio + menu overlay + launcher).
- **4 apps base** em `userspace/apps/`: editor, paint, fm, shell (Noite anterior).
- **Fix candidato** `invlpg` em `vm_map.c` (NÃO validado).
- **Bug conhecido** `RIP=+0x68` / `0x8` — flaky sob churn (~50%/clique no Iniciar).
- **Arquitetura ODK:** apps `.oep` (Ovsb Executable Program), extensões `.odk` (Ovsb Data Kernel).
- **`disk.img`:** contém `/BIN/FOOT`, `/BIN/SH`, `/BIN/DOOM`, `/APPS/DOOM1.WAD`, os 4 apps.
- **Locale:** `C.UTF-8`. Fonte `DejaVuSansMono.ttf`. `font8x8` embutida.

**⚠️ REGRA CRÍTICA:** **SEM SIMD.** O kernel não faz `fxsave`/`xsave` no `context_switch`. Tudo compilado com `-mno-sse -mno-mmx -mgeneral-regs-only`. **Não usar SIMD em nada.**

**⚠️ NÃO tocar em:** `fat32.c`, `switch.asm`, **compositor hand-rolled** (`wayland_compositor_lib.c`) — a não ser que a causa raiz comprovadamente esteja lá (aí PARE e reporte antes).

**⚠️ NÃO deletar:** `evidencias/`, `/tmp/bug2-rip-dump.log`, patches salvos.

---

## FASE 0 — SNAPSHOT E BRANCH

```bash
cd ~/OvsbOS
BRANCH="suprema/$(date +%Y%m%d-%H%M)"
git branch "$BRANCH" 2>/dev/null || true
git checkout "$BRANCH"
git commit --allow-empty -m "chore: snapshot tarefa suprema"
mkdir -p evidencias/suprema
sha256sum OvsbMk/kernel/*.c OvsbMk/kernel/*.asm 2>/dev/null > evidencias/suprema/manifest.sha256
date > evidencias/suprema/inicio.log
```

**Se `disk.img` não existir:** procure em `~/TipOS/`, `~/TerminalProprio/`, backups. Se achar, copie (não symlink). Se não achar, **PARE** e reporte.

**Se `TipOS.iso`/`OvsbOS.iso` não existir:** gere com `grub-mkrescue`. 

---

## PRIORIZAÇÃO (leia antes)

O tempo é **finito** (uma noite). Priorize nesta ordem:

1. **FASE 1 (Validar fix)** — se o fix `invlpg` resolve, isso é o mais importante da noite.
2. **FASE 2 (Apps base)** — consolidar o que já existe.
3. **FASE 3 (Imagens BMP/PPM)** — viável, alto valor.
4. **FASE 4 (Empacotador .oep)** — viável, alto valor.
5. **FASE 5 (Auditoria kernel)** — viável.
6. **FASE 6 (Dev inside: editor highlight, omake, oman)** — viável.
7. **FASE 7 (Emulador CHIP-8)** — viável; Game Boy **se sobrar**; GTA 1 = **documentar**.
8. **FASE 8 (Stress 2h+)** — **sempre rode**, em paralelo se possível.
9. **FASE 9 (Relatório)** — obrigatório.

**Se uma fase consumir >3h sem progresso, documente e pule.**

---

## FASE 1 — VALIDAR O FIX `invlpg` (CRÍTICO)

### 1.1 — Build limpo
```bash
make -C OvsbMk clean && make -C OvsbMk
cp OvsbMk/build/kernel.elf OvsbMk/iso/boot/kernel.elf
md5sum OvsbMk/build/kernel.elf OvsbMk/iso/boot/kernel.elf   # IGUAIS
grub-mkrescue -o OvsbOS.iso OvsbMk/iso
```

### 1.2 — Headless (sanidade)
```bash
pkill -9 -f qemu-system-x86_64
qemu-system-x86_64 -enable-kvm -boot order=d -cdrom OvsbOS.iso \
  -m 512M -drive file=disk.img,format=raw,index=0 \
  -serial file:/tmp/sup-headless.log -rtc base=localtime -no-reboot \
  -display none &
sleep 240
grep -E "PF-KILL|EXC#|PF@|SLAB-BAD|thexit" /tmp/sup-headless.log
grep -E "\[DESKTOP\] connected|wallpaper committed|bar committed" /tmp/sup-headless.log
pkill -9 -f qemu-system-x86_64
```
**Esperado:** zero faults.

### 1.3 — Reprodutor crítico (20 cliques no Iniciar)

Boot com display + QMP. Clicar no Iniciar **20 vezes**.
- **Antes:** ~50% de morte por clique.
- **Esperado:** 0 mortes em 20 cliques.

### 1.4 — Estresse adicional
- `foot` via menu 3×.
- `SCANTEST` 3×.
- Loop 100× de abrir/fechar `fm`.
- Aguardar 60s.

### 1.5 — Resultado
```
FASE 1:
  headless:        PASS | FAIL
  reprodutor 20x:  PASS | FAIL (mortes: N)
  estresse:        PASS | FAIL
  faults:          <saída>
  CONCLUSÃO:       fix resolveu | fix insuficiente | fix não resolve
```

**Se o fix resolver:** salve `/tmp/fix-invlpg-validado.log`. **Aplicar em `main` ao final (se todas as outras fases passarem).**

---

## FASE 2 — APPS BASE (consolidar)

### 2.1 — Build limpo
```bash
cd ~/OvsbOS/userspace/apps
make clean && make 2>&1 | tee /tmp/sup-apps-build.log
```

### 2.2 — Validar cada app (editor, paint, fm, shell)
- Compila sem erro/warning?
- Roda headless (via `execve` no shell)?
- Roda com display (menu → app)?
- Fecha limpo (sem zumbi)?
- Screenshot → `evidencias/suprema/apps/<nome>.ppm`.

### 2.3 — Corrigir bugs encontrados (se triviais)
- Não deixe crash.
- Não deixe zumbi.
- Não deixe warning (`-Wall -Wextra -Werror`).

### 2.4 — Resultado
```
FASE 2:
  build:   PASS | FAIL
  editor:  roda PASS|FAIL | fecha PASS|FAIL
  paint:   ...
  fm:      ...
  shell:   ...
```

---

## FASE 3 — BIBLIOTECA DE IMAGENS

**Prioridade: BMP + PPM (obrigatório). PNG/JPEG (bônus, se viável).**

### 3.1 — BMP + PPM (obrigatório)

Crie `userspace/libs/image/`:
- **BMP** (24/32-bit, sem compressão): `img_bmp_load`, `img_bmp_save`.
- **PPM (P6)**: `img_ppm_load`, `img_ppm_save`.
- **API unificada:** `img_load(path) -> img*`, `img_save(img*, path)`, `img_free(img*)`.
- **Ops:** escala, crop, blend alpha básico.
- **Makefile estático** (sem dep externa).

### 3.2 — PNG (bônus)

- **Decoder PNG mínimo** (IHDR + IDAT + filtros).
- Use `zlib` **estático** se existir no host; senão, **implementar inflate mínimo** (~500 linhas).
- Se inviável em 2h → **documentar bloqueio** e pular.

### 3.3 — JPEG (bônus, se PNG passar)

- **Decoder JPEG baseline** (sem progressive).
- Se inviável → **documentar bloqueio** e pular.

### 3.4 — Integrar nos apps
- **paint:** abrir BMP/PPM (PNG/JPEG se feito), salvar BMP/PPM.
- **editor:** visualizar imagens pequenas (hex/ascii).
- **fm:** preview de imagem ao selecionar.

### 3.5 — Resultado
```
FASE 3:
  BMP:  ler PASS|FAIL | escrever PASS|FAIL
  PPM:  ler PASS|FAIL | escrever PASS|FAIL
  PNG:  PASS|FAIL (ou: bloqueado, motivo X)
  JPEG: PASS|FAIL (ou: bloqueado, motivo X)
  paint: PASS|FAIL
  fm:    PASS|FAIL
```

---

## FASE 4 — EMPACOTADOR `.oep` / `.odk`

**Contexto:** apps `.oep` (Ovsb Executable Program); extensões `.odk` (Ovsb Data Kernel). Container + manifesto.

### 4.1 — Ferramenta `oep-pack`
Crie `userspace/tools/oep-pack/`:
- Empacota app ELF + manifesto JSON + assets num container `.oep`.
- **Formato:** header mágico `OEP\0`, manifesto JSON, seções (código, dados, assets).
- **CLI:** `oep-pack app.elf assets/ -o app.oep`.

### 4.2 — Ferramenta `oep-unpack` + `oep-list`
- Extrai um `.oep` em disco.
- Lista conteúdo (`oep-list app.oep`).

### 4.3 — Loader básico
- Userspace: abre `.oep`, mapeia ELF, `execve`.
- Não precisa integrar ao kernel — wrapper é suficiente.

### 4.4 — Empacotar os 4 apps
- Empacotar editor, paint, fm, shell em `.oep`.
- Testar: `.oep` roda igual ao binário cru.

### 4.5 — Resultado
```
FASE 4:
  oep-pack:   PASS|FAIL
  oep-unpack: PASS|FAIL
  loader:     PASS|FAIL
  apps empacotados: editor | paint | fm | shell
```

---

## FASE 5 — AUDITORIA DE INTEGRIDADE DO KERNEL

Para **cada** arquivo `.c`, `.h`, `.asm`, `.S` em `OvsbMk/kernel/`:

1. **Syntax check** isolado.
2. **Procura por:**
   - Ponteiro sem NULL check.
   - `memcpy`/`strcpy` sem bound.
   - Variável não inicializada.
   - Aritmética de ponteiro suspeita.
   - `TODO`/`FIXME`/`HACK`/`XXX`.
   - Código morto/inalcançável.
3. **Selo:** `OK` / `SUSPEITO` / `CRÍTICO`.
4. **Relatório:** `evidencias/suprema/auditoria-kernel.md`.

**Foco especial:** o **trampolim asm do syscall entry/exit** (~6 instruções entre epílogo C e `iretq`). Desmonte, comente cada instrução, verifique o frame (ordem push/pop, alinhamento, canário, `SS/RSP/CS/RFLAGS`).

### 5.1 — Melhorias estruturais
Com base na auditoria:
- **Guards de NULL** em pontos `SUSPEITO`/`CRÍTICO`.
- **Bounds checking** em cópias.
- **Comentários** nos trechos asm críticos.
- **Testes unitários** por syscall (se houver framework).

**Regra:** melhoria arriscada → **NÃO aplique**, documente em `evidencias/suprema/melhorias-recomendadas.md`.

### 5.2 — Resultado
```
FASE 5:
  arquivos auditados: N
  OK:        N
  SUSPEITO:  N
  CRÍTICO:   N
  melhorias aplicadas: N
  melhorias recomendadas: N
```

---

## FASE 6 — FERRAMENTAS DE DEV DENTRO DO SISTEMA

**Objetivo:** poder desenvolver **no próprio OvsbOS**.

### 6.1 — Editor com syntax highlight
- Melhorar o `editor` (Fase 2): highlight para C, ASM, shell.
- Busca/substituição, numeração de linhas.

### 6.2 — Compilador/assembler (se viável)
- **TCC** estático se houver versão compilável.
- Ou **NASM** estático.
- Se ambos inviáveis → **mini-assembler de brinquedo** (`mov`, `add`, `jmp`) como demonstração.

### 6.3 — Ferramenta de build (`omake`)
- `make` mínimo: lê `Makefile`, executa `cc`/`as`/`ld`.
- Ou script shell equivalente.

### 6.4 — Documentação embutida (`oman`)
- `man` mínimo: lê `docs/`, exibe.

### 6.5 — Resultado
```
FASE 6:
  editor highlight:  PASS|FAIL
  compilador:        PASS|FAIL (ou bloqueado, motivo X)
  mini-assembler:    PASS|FAIL
  omake:             PASS|FAIL
  oman:              PASS|FAIL
```

---

## FASE 7 — EMULADOR

**Realismo:** GTA 1 é **irrealista** numa noite. Foque no viável.

### 7.1 — Alvos (ordem de viabilidade)
- **(a) CHIP-8** — trivial (~200 linhas). **Implemente.**
- **(b) Game Boy (DMG)** — médio (~2000 linhas). **Se sobrar tempo.**
- **(c) NES** — grande (~3000 linhas). **Improvável.**
- **(d) DOSBox** — sem libs. **Inviável.**
- **(e) GTA 1 (PS1/DOS)** — **documente como objetivo de longo prazo.**

### 7.2 — Implementar CHIP-8
- Opcodes, display (64×32), input (16 teclas), som (opcional).
- Rode no desktop (janela Wayland) ou em fullscreen no framebuffer.
- Inclua **1-2 roms livres** (domínio público) pra testar.

### 7.3 — Game Boy (se viável)
- CPU (Z80-like), PPU, timer, input.
- Sem BIOS proprietária — use **boot ROM livre**.

### 7.4 — Resultado
```
FASE 7:
  CHIP-8:    PASS|FAIL
  Game Boy:  PASS|FAIL (ou bloqueado)
  GTA 1:     documentado como objetivo de longo prazo (motivo)
```

---

## FASE 8 — STRESS DE ULTRA POTÊNCIA (2h+, em paralelo se possível)

Crie `evidencias/suprema/stress.sh` que:

1. Abre/fecha `fm` **1000×** em loop.
2. Navega em diretórios com muitos arquivos.
3. Abre `editor`, digita 10 KB, salva, fecha.
4. Abre `paint`, desenha 100 formas, salva, fecha.
5. Loop de **abrir/fechar `.oep`** 1000×.
6. Loop de **carregar/salvar imagens** (BMP/PPM/PNG/JPEG) 500×.
7. Loop do **emulador** rodando 30 min contínuo.
8. Tudo com **churn de memória** (`malloc`/`free` paralelo).

Rode por **mínimo 2 horas** (ou até acabar). Capture **cada crash** com: `RIP`, `RSP`, `RBP`, registradores, stack dump, frame.

Salve em `evidencias/suprema/crashes/crash-<timestamp>.log`.

**Correlação:** se crash sempre no mesmo `RIP`, mapeie com `readelf -l` / `objdump -d` / `nm`.

### 8.1 — Resultado
```
FASE 8:
  crashes: N
  RIPs únicos: <lista>
  padrões: <descrição>
```

---

## FASE 9 — RELATÓRIO FINAL (OBRIGATÓRIO)

Gere `~/relatorio-supremo-$(date +%Y%m%d).md`:

1. **Resumo executivo** (1 parágrafo).
2. **FASE 1:** fix `invlpg` resolveu? (o mais importante).
3. **FASE 2:** apps — status, screenshots, LOC, memória.
4. **FASE 3:** imagens — BMP/PPM/PNG/JPEG (status, integração).
5. **FASE 4:** empacotador `.oep` (pack/unpack/loader).
6. **FASE 5:** auditoria — selos por arquivo, achados, melhorias.
7. **FASE 6:** dev inside — editor highlight, compilador, omake, oman.
8. **FASE 7:** emulador — CHIP-8 / Game Boy / GTA 1 (status).
9. **FASE 8:** crashes — quantidade, padrões, RIPs únicos.
10. **Integração desktop:** ícones, memória, zumbis.
11. **Próximos passos** (máx. 5).
12. **O que NÃO foi feito e por quê** (seja honesto).

Markdown, com tabelas, blocos de código, links pras evidências.

---

## PONTOS DE PARADA

PARE e reporte se:

1. `disk.img` sumir e não houver backup.
2. `md5sum` do kernel divergir entre `build/` e `iso/boot/`.
3. Kernel não bootar em 300s.
4. `PF@|EXC#|SLAB-BAD` **novo** aparecer além do conhecido `+0x68`.
5. Causa raiz comprovadamente em `fat32.c` ou compositor hand-rolled → PARE.
6. Algo exigir SIMD → PARE.
7. `foot` precisar recompilar (tem patch em `input.c`).
8. Após 2 tentativas, o fix `invlpg` não resolver → **documente e siga** (não insista).
9. Uma fase consumir >3h sem progresso → **documente e pule**.

---

## REGRAS GERAIS

- **SEM SIMD.** `-mno-sse -mno-mmx -mgeneral-regs-only`.
- **NÃO deletar** evidências (`evidencias/`, `/tmp/bug2-rip-dump.log`).
- **NÃO commitar sem `make clean && make` antes.**
- Se um teste travar >10 min, mate e siga.
- Se o QEMU travar, reinicie e continue de onde parou.
- Preserve evidências **antes** de consertar.
- **Realismo > ambição.** Se PNG/Game Boy/TCC não derem, **documente e siga**.
- Prefira código funcionando a código bonito.
- Um commit por fase, **sem push**.
- Ao final: `git log --oneline > evidencias/suprema/git-log.txt`.
- Deixe o branch `suprema/*` pronto pra revisão matinal.
- **Se o fix `invlpg` resolver na FASE 1, e todas as outras fases passarem: merge `main` + `suprema/*`, com mensagem clara.**

**Boa noite. Trabalhe até o fim. Não pare pra perguntar.**