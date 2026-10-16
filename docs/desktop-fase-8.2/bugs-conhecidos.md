# Bugs conhecidos (com evidência)

## `+0x68` (kernel, ABERTO)
Fetch ring-3 em página presente (`err=0x15`), RIP em região mapeada
supervisor (`PD=...E3`, bit U zerado), RBP pequeno, OP=`mov [rsp-32],0x1b`
(red herring: dump lê a memória do kernel). USTK mostra stack saudável +
frame iretq íntegro exceto slot RIP. Mata scan-filhos, compositor e apps
sob churn; desktop sobrevive (fork-isolation). Hipótese líder: corrupção
do RIP de retorno em regime (investigação em `evidencias/` + USTK no
kernel). Fix `invlpg` em `vm_mprotect` NÃO resolveu (documentado).

## foot/DOOM sem teclado (ARQUITETURAL)
Layers não recebem kbd no compositor (só xdg com foco). Apps lançados do
menu que precisam de teclado parecem "mortos". Sem fix rápido.

## Mouse (MITIGADO)
Pacotes PS/2 com overflow aplicados como movimento (pulos "sozinhos") +
bytes obsoletos no init desalinhavam o ciclo. Fix: drop de overflow +
drain AUX no init (`mouse.zig`).

## Performance (MITIGADO, pendente compositor)
Serial 38400→115200; barra commita só no clock; cursor fast-path +
log-gating no compositor (aplicado após liberação). Preempção real por
timer existe (não validada em boot completo — ver `pit.c`/`sched_active`).

## Congelamento total
Spinner puro em ring 3 nunca largava a CPU (scheduler cooperativo);
`cli;hlt` em EXC ring-0 também congela (tela parada = "QEMU crashou").
Preempção (acima) + auto-cura do desktop (relança compositor, máx. 3x)
mitigam. QEMU morrer de verdade é host-side (OOM/GTK) — capturar stderr.
