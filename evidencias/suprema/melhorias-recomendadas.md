# Melhorias recomendadas (NAO aplicadas — arriscadas sem sessao QEMU)

1. `signal_dispatch_irq` (`OvsbMk/kernel/signal.c:248-286`): indices errados
   (RFLAGS como RIP em `adapted[15]`, SS como RFLAGS em `adapted[16]`;
   gate deveria ler CS em `registers[16]`, nao `[18]`). Corrigir HABILITA
   entrega de sinal via timer (hoje morta) — mudar comportamento runtime.
   Requer teste guest de sinais antes.
2. Validacao de user-RSP canonico/user no trampolim antes das 5 escritas
   do frame iretq (RSP selvagem = #PF em CPL0). Requer teste guest.
3. Probes de memoria user sem validacao (`signal.c:225,302,306`,
   dump `OP=` em `idt.c:248-252`): adicionar probe com fault-handler ou
   `vm_map_covers` antes de dereferenciar.
4. Unificar layouts de frame (syscall vs irq0 vs isr_common) ou ao menos
   `static_assert` de offsets + documentacao (iniciado em `docs/`).
5. Testes unitarios por syscall: sem framework guest hoje; sugerido
   `OvsbMk/tests/` rodando asserts via serial.

# Melhorias aplicadas nesta noite

- `vm_map.c:vm_mprotect`: loop `invlpg` + `__sync_synchronize` (fix TLB stale).
- `env.c:alias_unset`: `strcpy` -> `memcpy` exato de linha fixa.
- Comentarios de auditoria no trampolim (`syscall_entry.asm`).
