# Preempção por timer: tentativa, falha e lições (2026-10-16)

## Motivação
Spinner puro em ring 3 (ex.: CHURN_TEST) nunca larga a CPU: scheduler é
cooperativo (só troca em `schedule()` via kernel_main/epilogue/exit) →
tela congelada, mouse morto, serial mudo = "sistema trava".

## Tentativa
`schedule()` no fim de `timer_tick_handler` (só roda no path user do
irq0: current=user, RBX=frame, g_in_syscall=0) + guarda anti-reentrância
(`in_sched`) + gate de boot (`sched_active`, ligado no loop kernel_main).

## Falha (bisseção limpa)
Com a chamada ativa: boot morre em `#GP(0)` no `iretq` do pop-path do
`context_switch` durante setup dos pids (frame IRET todo deslocado:
só RIP válido). Sem a chamada (resto idêntico): boota.
Gate `sched_active` NÃO salvou (falha ocorre mesmo com ele).

## Hipóteses restantes (não provadas)
1. RBX não é o frame em algum caminho que chega ao schedule via timer
   (ex.: tick aninhado em momento inesperado).
2. KRNL_RSP stale + pops em área reutilizada por syscalls no mesmo kstack.
3. Janela bring-up maior que o gate (processos marcados READY antes do
   frame forjado — `state=READY` vem antes da forja em spawn/fork/clone!).

## Estado deixado
- Chamada REMOVIDA (flag-only, como antes). Guarda `in_sched` mantida
  (inócua, protege futuro). `sched_active` mantido (inócuo).
- Próximo passo sugerido: marcar READY por ÚLTIMO em spawn/fork/clone +
  re-tentar com trace do primeiro switch.
