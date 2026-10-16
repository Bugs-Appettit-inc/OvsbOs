# Hipotese final — bug `+0x68` (pid 9, user-code, flaky sob churn)

## 1. Evidencia bruta (preservada em `bug2-rip-dump.log.bak`)

```
PF@0x08027478 ... CS=0x1B err=0x15 RIP=0x08027478 U_RSP=0x1F9FD8F0
RBP=0x2C0 RDI=0xFFFFFF9C RSI=0x1F9FD9C0 RDX=0 RCX=0x00444D67 ...
PD[0x40]=0x080000E3 PDhi=0x0 ... OP= 48 C7 44 24 E0 1B
[PFIX] cobre=0 us=...  ->  [PF-KILL] exc=0xE pid=9 rip=0x08027478 err=0x15
```

Contexto: pid 9 fazia `openat` (n=0x101) em loop sobre `/BIN/*`
(menu INICIAR aberto — clique sob churn). `RCX=0x00444D67` = endereço de
retorno de syscall no `.text` (binário principal íntegro).

## 2. Decodificacao

- `err=0x15` = `10101b`: P=1 (presente), W=0 (leitura), U=1 (ring 3),
  I=1 (**instruction fetch**). Ou seja: fetch em modo usuário numa página
  que as tabelas dizem presente.
- Walk: `PD[0x40]=0x080000E3` (huge page 2 MB, P+RW+U, bit 0x80=PS),
  `PDhi=0` ⇒ **bit NX (63) LIMPO** — a página é executável nas tabelas.
- `OP=` (`idt.c:247-252`, 6 bytes lidos no RIP usuário): a instrução que
  falhou é `48 C7 44 24 E0 1B …` = **`mov qword [rsp-0x20], 0x1b`**.
- Correlação nova: esses 6 bytes são **idênticos ao encoding da instrução
  (2) do trampolim de syscall** (`syscall_entry.asm`, offset 0x68 no
  `.text`: `mov [rsp-0x20],0x1b` = push do CS). O userland OvsbOS usa
  `int 0x80` e não contém esse padrão em nenhum binário legítimo.

## 3. Hipótese principal: TLB stale após `mprotect` sem `invlpg`

- `vm_mprotect` (`OvsbMk/kernel/vm_map.c:689-704`) + callback
  `pte_set_prot` (`:556-573`) alteram bits W/U/**NX** das PTEs **sem nenhum
  `invlpg`** (nem no callback, nem loop após o walk). Compare: `vm_munmap`
  TEM loop `invlpg` + `__sync_synchronize`; `pte_absent` TEM `invlpg`.
- Efeito: a TLB continua aplicando as permissões ANTIGAS. Se a página foi
  `mprotect`ed (RELRO do loader dinâmico, transições W→X), a CPU pode
  gerar `#PF` de fetch (I=1) mesmo com as tabelas corretas — exatamente a
  assinatura observada (tabelas dizem X, CPU diz NX).
- Flakiness: depende de a entrada TLB estar cacheada ou já evictada no
  momento do fetch ⇒ ~50%/clique sob churn (loader + menu fazem
  mmap/mprotect o tempo todo), 0% idle. **Encaixa em tudo.**
- Por que a hipótese "TLB-via-invlpg" foi (mal) refutada antes:
  os caminhos de *demand paging* (`idt.c:316`) e *USFIX* (`:369`) TÊM
  `invlpg` — mas o caminho **`mprotect` NÃO tem**. Testou-se o lugar errado.
- Teste decisivo (barato, sem QEMU): inspeção estática acima + `grep invlpg`
  (feito). Teste dinâmico: aplicar `evidencias/iretq-probe.diff`? Não —
  o teste correto é adicionar o loop `invlpg` em `vm_mprotect`, rodar o
  stress no guest e ver o crash sumir (Fase 5 implementa o fix).

## 4. Para onde aponta o RIP desviado (0x08027478)?

- `0x08027478` está na região de mmap/stack (huge page 2 MB a partir de
  `0x08000000`); o `.text` do processo é `0x00444D67`.
- `RBP=0x2C0` (lixo) + `R14=0x690` sugerem **stack smash / frame stale**:
  o RIP foi parar em bytes que decodificam como `mov [rsp-32],0x1b`.
- Candidato concreto para a corrupção contribuinte: `signal_dispatch_irq`
  (`signal.c:248-286`) usa índices errados (RFLAGS como RIP, SS como
  RFLAGS — ver `auditoria-kernel.md` A2). Sinais via timer são raros
  (o gate `[18]&3` quase sempre fecha), mas quando passam, plantam um
  retorno selvagem — consistente com "RIP=+0x68/assinatura 0x8" flaky.
- Ou seja: **causa do PF = TLB stale (mprotect)**; **conteúdo do RIP =
  corrupção secundária** (stack/sinal). O fix do `invlpg` elimina o crash
  mesmo sem explicar 100% do desvio — e o desvio sozinho (sem NX stale)
  seria só um salto para página executável (provável SIGSEGV comum, não
  este PF).

## 5. Mapeamento do `+0x68`

- `+0x68` = offset 0x68 no `.text` do objeto do trampolim = exatamente a
  instrução `mov [rsp-0x20],0x1b` (ver disassembly em `auditoria-kernel.md`
  A4). Se o relato "`RIP=+0x68`" refere-se a offset em binário dinâmico,
  use `readelf -l /BIN/<suspeito>` + `objdump -d` e procure o mesmo padrão
  de bytes `48 C7 44 24 E0 1B` — se algum loader/stub o contiver, ele é o
  dono do RIP; senão, vale a leitura acima (RIP desviado).

## 6. Experimento 4.1 (pronto, não aplicado — requer sessão QEMU)

`evidencias/iretq-probe.diff`: insere `int3` imediatamente antes do
`iretq` em `syscall_entry.asm`. Previsão:
- crash muda de `+0x68` para o endereço do `int3` ⇒ culpa ANTES do iretq
  (frame malformado no trampolim);
- crash NÃO muda ⇒ culpa DEPOIS (user-code pós-iretq — corrobora §3+§4).
Não aplicado esta noite para não invalidar o snapshot de teste; o diff
compila (`nasm -f elf64` OK) e está pronto para a sessão com QEMU.
