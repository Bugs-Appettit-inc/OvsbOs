# Coerência de TLB — regra do projeto

**Toda mutação de entrada de página (PTE/PD) em um `pml4` vivo DEVE ser
seguida de `invlpg` na VA afetada + `__sync_synchronize()`.**

Histórico: `vm_mprotect` alterava bits W/U/NX sem flush; a TLB servia as
permissões antigas e processos morriam com `#PF err=0x15` (fetch em página
"presente") de forma flaky sob churn — bug `+0x68`
(ver `evidencias/hipotese-final.md`). O fix espelha o padrão já usado em
`vm_munmap` e `pte_absent`.

Checklist para código novo que toca em tabelas:
1. `pml4_walk_ptes` (ou escrita direta) → loop `invlpg` por página (4 KiB;
   para huge pages, uma por 4 KiB coberto é o padrão conservador atual).
2. `__sync_synchronize()` após o loop.
3. Nunca confiar em "a TLB vai evictar sozinha".
