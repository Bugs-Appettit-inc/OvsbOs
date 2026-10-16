# FASE 1 — Caça ao +0x68 com ring buffer (resultado: NÃO morto, mas encurralado)

## Tentativa
Ring de 64x8 qwords no `syscall_entry` (save/restore/iretq-frame + pid +
tsc), dump no PF de assinatura `err&0x15==0x15`. Removido após teste.

## Resultado inesperado (importante!)
O próprio ring CAUSOU `#GP` ring-0: pushes na red-zone do usuário no
call-site do iretq + ~20 stores por syscall desestabilizaram o retorno
(boot travava / #GP em `iretq` cercado pelo macro). Revertido na hora;
sem o ring, o #GP sumiu. USTK read-only mantido (nunca causou nada).

## Dado novo mais valioso (do USTK estendido, 16 qwords de RSP-64)
Frame íntegro (CS/RFLAGS/RSP/SS) exceto slot RIP, stack saudável, GPRs
pequenos — em TODOS os crashes (scan-child, pai, compositor, fresh-exec).

## Suspeito refinado (não provado)
Retorno via pop-path do `context_switch` com KRNL_RSP stale (apontando
p/ área de C-stack reutilizada, com endereços de retorno do kernel
0x0802xxxx), enquanto o dump na stack do usuário mostra um frame
"stale-but-plausível" da iteração anterior do loop (mesma profundidade
→ RSP/CS/RFLAGS/SS coincidem; slot RIP é lixo de red-zone). Ou seja:
o USTK pode ser distração — o frame AUTORITATIVO (kernel) nunca foi
dumpado. Próximo passo sugerido: dumpar KRNL_RSP + 20 qwords no PF
(leitura só-kernel, segura) e comparar com o frame do usuário.
Também: marcar READY por ÚLTIMO em spawn/fork/clone (hoje READY vem
antes da forja — janela p/ preempção futura).
