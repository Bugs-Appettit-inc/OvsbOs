# Trampolim de syscall — notas de auditoria

Entradas: `syscall` (MSR LSTAR → `syscall_entry` em
`OvsbMk/kernel/syscall_entry.asm`) e `int 0x80` (`syscall_isr` em
`OvsbMk/kernel/idt.asm`). Ambas retornam via **`iretq`** (sysretq não
fecha com esta GDT — ver comentário no .asm).

## Layouts de frame (NÃO são iguais!)

| Porta | regs[0..14] | regs[15] | regs[16] | extra |
|---|---|---|---|---|
| `syscall_entry` | rax..r15 (rax em [0]) | RIP | RFLAGS | — |
| `syscall_isr` | idem | — | — | frame iretq de 5 qwords da CPU abaixo |
| `irq0` (.from_user) | r15..rax (r15 em [0]) | RIP | CS | RFLAGS[17] RSP[18] SS[19] |
| `isr_common` | r15..rax | int_no | err | RIP[17] CS[18] RFLAGS[19] RSP[20] SS[21] |

Quem consome cada layout: `context_switch` (pop na ordem syscall),
`signal_dispatch_syscall` (ordem syscall), `signal_dispatch_irq`
(converte irq→syscall — **com índices errados para RIP/RFLAGS, ver
`evidencias/auditoria-kernel.md` A2, correção pendente por risco**),
diagnóstico de PF em `idt.c` (ordem isr_common).

## As 6 instruções cegas (ver A4 na auditoria)

Desmontado e comentado em `evidencias/auditoria-kernel.md`.
Regra de ouro: nunca reordenar os 5 `mov` para depois do `sub`
sem revisar a análise de race com outro thread do mesmo `mm`.
