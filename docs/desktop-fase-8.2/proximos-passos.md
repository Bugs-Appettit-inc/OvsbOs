# Próximos passos (Fase 8.3/8.4)

1. Validar preempção por timer em boot completo (risco: janela
   bring-up; gate `sched_active` + READY-por-último precisam de teste).
2. Caçar o `+0x68` com o trigger 100% (scan-fork + HMP): candidato atual
   é corrupção do slot RIP no retorno de syscall em regime.
3. Roteamento de teclado p/ layers OU launcher que avisa ("app precisa
   de teclado — abra pelo shell").
4. Damage tracking total no compositor (hoje: cursor fast-path).
5. Busybox-like: `cp/mv/ls` no shell já existem; amarrar `help` + `man`.
