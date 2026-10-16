# Auditoria do kernel — suprema (OvsbMk/kernel/, 40 arquivos)

Metodo: `gcc -fsyntax-only` (falhas = include-path freestanding, build usa -I proprio), `nasm -f elf64` p/ asm, varredura de padroes. Auditoria anterior (noite, tree inteira) em `evidencias/auditoria-kernel.md`.

Totais: 40 | OK=10 SUSPEITO=29 CRITICO=0

| Arquivo | LOC | syntax | Selo | Achados |
|---|---|---|---|---|
| OvsbMk/kernel/boot64.asm | 257 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/console.c | 210 | OK | OK | 0 |
| OvsbMk/kernel/console.h | 34 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/drivers/tty.c | 581 | FALHA-INCLUDE | SUSPEITO | 1 |
| OvsbMk/kernel/drivers/tty.h | 129 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/dynlink.c | 453 | OK | OK | 0 |
| OvsbMk/kernel/env.c | 112 | OK | CRÍTICO | 2 |
| OvsbMk/kernel/env.h | 41 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/idt.asm | 288 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/idt.c | 498 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/idt.h | 46 | N/A | OK | 0 |
| OvsbMk/kernel/kernel.c | 140 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/mach_o.c | 343 | OK | OK | 0 |
| OvsbMk/kernel/mach_o.h | 93 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/memory.c | 1281 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/memory.h | 70 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/owt_app.c | 43 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/pic.c | 64 | OK | OK | 0 |
| OvsbMk/kernel/pit.c | 67 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/pit.h | 28 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/process.c | 1272 | OK | OK | 0 |
| OvsbMk/kernel/process.h | 129 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/rtc.c | 57 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/rtc.h | 31 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/serial.c | 56 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/serial.h | 26 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/shell.c | 705 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/shell.h | 21 | N/A | OK | 0 |
| OvsbMk/kernel/signal.c | 322 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/signal.h | 49 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/switch.asm | 252 | OK | SUSPEITO | 2 |
| OvsbMk/kernel/syscall.c | 5973 | OK | SUSPEITO | 5 |
| OvsbMk/kernel/syscall.h | 1711 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/syscall_entry.asm | 133 | OK | OK | 0 |
| OvsbMk/kernel/tss.c | 114 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/tss.h | 24 | N/A | OK | 0 |
| OvsbMk/kernel/utils.c | 32 | OK | SUSPEITO | 1 |
| OvsbMk/kernel/utils.h | 29 | N/A | SUSPEITO | 1 |
| OvsbMk/kernel/vm_map.c | 786 | OK | OK | 0 |
| OvsbMk/kernel/vm_map.h | 91 | N/A | SUSPEITO | 1 |

## Detalhes nao-OK
### OvsbMk/kernel/boot64.asm — SUSPEITO
- L223: marcador (;   Antes era 0x87 (com U): TODO o RAM baixo (inclusive frames l)

### OvsbMk/kernel/console.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/drivers/tty.c — SUSPEITO
- syntax: include-path freestanding (build usa -I proprio)

### OvsbMk/kernel/drivers/tty.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/env.c — CRÍTICO
- L91: copia sem bound (strcpy(alias_name[j], alias_name[j+1]);)
- L92: copia sem bound (strcpy(alias_val[j], alias_val[j+1]);)

### OvsbMk/kernel/env.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/idt.asm — SUSPEITO
- L237: marcador (; Salvamos TODOS os 15 GPRs (rax..r15) pra C poder mexer sem med)

### OvsbMk/kernel/idt.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/kernel.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/mach_o.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/memory.c — SUSPEITO
- L220: marcador (/* varre TODOS os processos (não só o CR3 atual) */)
- L350: marcador (/* FASE5: marca ALLOC|USER em TODOS os frames do bloco (ordem só)

### OvsbMk/kernel/memory.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/owt_app.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/pit.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/pit.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/process.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/rtc.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/rtc.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/serial.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/serial.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/shell.c — SUSPEITO
- L402: marcador (/* varre TODOS os pares do auxv (do início ao AT_NULL)~~ */)
- L698: marcador (/* ♥ TODO: adicionar suporte a argumentos pra exec ~ argv futura)

### OvsbMk/kernel/signal.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/signal.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/switch.asm — SUSPEITO
- L32: marcador (;   a partir de MOV CR3, rax TODOS os endereços são reinterpreta)
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/syscall.c — SUSPEITO
- L1383: marcador (* Varre as fd tables de TODOS os processos vivos~ */)
- L1713: marcador (* readv com 2 iovecs — sem handler TODO fread retornava)
- L1839: marcador (/* FASE6-evdev: pacote SÓ é consumido se TODOS os seus)
- L3628: marcador (* *** stack smashing detected *** → exit 134 em TODO)
- L5768: marcador (/* FASE5-fix: entrega TODOS os pendentes que couberem no)

### OvsbMk/kernel/syscall.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/tss.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/utils.c — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/utils.h — SUSPEITO
- sem checagem NULL visivel no arquivo

### OvsbMk/kernel/vm_map.h — SUSPEITO
- sem checagem NULL visivel no arquivo

## Trampolim syscall entry/exit (ponto cego)
Desmontado de `OvsbMk/kernel/syscall_entry.asm` (bytes identicos pos-comentario):
```
63: 48 89 4c 24 d8   mov [rsp-0x28],rcx   ; (1) RIP return original
68: 48 c7 44 24 e0   mov [rsp-0x20],0x1b   ; (2) CS=0x1B ring3
71: 4c 89 5c 24 e8   mov [rsp-0x18],r11   ; (3) RFLAGS return
76: 48 89 64 24 f0   mov [rsp-0x10],rsp   ; (4) RSP user original
7b: 48 c7 44 24 f8   mov [rsp-0x8],0x23    ; (5) SS=0x23 ring3
84: 48 83 ec 28      sub rsp,0x28         ; (6) publica o frame
88: 48 cf            iretq                ; consome 5 qwords
```
Ordem push correta; `sub` por ultimo; IRQ/NMI usam kernel-stack via TSS (sem clobber); sem validacao de user-RSP canonico (ver melhorias-recomendadas).
