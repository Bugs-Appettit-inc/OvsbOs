# TAREFA NOTURNA — MUSE SPARK: DESKTOP + APPS BASE + AUDITORIA DE KERNEL + STRESS TEST

Você é um engenheiro de sistemas sênior trabalhando sozinho durante a noite.
Execute TODAS as fases abaixo em ordem, sem parar para pedir confirmação.
Se travar numa fase, documente o bloqueio e SIGA para a próxima.
Ao final, gere um relatório único em `~/relatorio-noturno-$(date +%Y%m%d).md`.

Contexto do projeto:
- Kernel custom (commit base: 6fa638cb) rodando em QEMU (OEMU).
- Existe um bug conhecido: crash em user-code, `pid 9, RIP=+0x68`,
  assinatura `0x8`, ~50%/clique sob churn, 0% idle. Flaky.
- Já foram refutadas 13 hipóteses (save global, frame-stale, stack 8KB,
  glibc/malloc, open/read, pager, buddy/SLAB, HW breakpoint, rsp0/TSS,
  stack-smash, IRQ-window, copy_to_user, red-zone, signal, pool-overlap,
  TLB-via-invlpg, entry-chx, demand-zero, FAT-leitura, canário, DRx).
- Ponto cego identificado: as 6 instruções asm entre o epílogo C do
  syscall handler e o `iretq`. Nenhum hook em C alcança.
- Arquivo de evidência: /tmp/bug2-rip-dump.log (preservar, não deletar).

============================================================
FASE 0 — SETUP E SNAPSHOT
============================================================
1. Crie branch `noite/audit-$(date +%Y%m%d-%H%M)` no repo do kernel.
2. Faça commit vazio "chore: snapshot pré-auditoria noturna".
3. Copie /tmp/bug2-rip-dump.log para ./evidencias/bug2-rip-dump.log.bak
4. Registre hash SHA256 de TODOS os arquivos do kernel em ./evidencias/manifest.sha256
5. Rode `dmesg > ./evidencias/dmesg-inicial.log` se possível.

============================================================
FASE 1 — AUDITORIA DE INTEGRIDADE DO KERNEL
============================================================
Para CADA arquivo .c, .h, .S, .asm no tree do kernel:
1. Verifique se compila isoladamente (syntax check).
2. Procure por:
   - Acesso a ponteiros sem checagem de NULL
   - `memcpy`/`strcpy` sem bound
   - Uso de variáveis não inicializadas
   - Aritmética de ponteiro suspeita (casts, offsets mágicos)
   - Comentários TODO/FIXME/HACK/XXX
   - Código morto ou inalcançável
3. Marque cada arquivo com selo: OK / SUSPEITO / CRÍTICO.
4. Gere ./evidencias/auditoria-kernel.md com achados.

Foco especial (ponto cego do bug):
- Localize o trampolim asm do syscall entry/exit.
- Desmonte o trecho entre o `ret` do handler C e o `iretq`.
- Comente cada uma das ~6 instruções.
- Verifique se o frame de retorno está sendo construído corretamente
  (ordem de push/pop, alinhamento de stack, canário, SS/RSP/CS/RFLAGS).
- Procure por qualquer caminho onde o frame possa ser lido antes de
  estar 100% escrito (race com IRQ? NMI? SMI?).

============================================================
FASE 2 — APPS BASE DO DESKTOP
============================================================
Construa, no diretório ./userspace/apps/, os seguintes apps.
Todos devem:
- Ser standalone (sem dependência de libs externas além de syscalls).
- Ter Makefile próprio.
- Compilar limpo com -Wall -Wextra -Werror.
- Ter versão CLI E GUI (se o desktop suportar).

2.1 EDITOR DE TEXTO (`editor/`)
    - Abrir, editar, salvar arquivos
    - Navegação com setas, Home/End, PageUp/PageDown
    - Busca (Ctrl+F), salvar como (Ctrl+S), sair (Ctrl+Q)
    - Suporte a UTF-8 básico
    - Sem crash se arquivo não existir ou for binário

2.2 PAINT (`paint/`)
    - Canvas com dimensões configuráveis
    - Pincel, borracha, linha, retângulo, elipse, balde de tinta
    - Paleta de cores (mínimo 16 cores)
    - Salvar/carregar em formato próprio (BMP ou PPM)
    - Desfazer (Ctrl+Z) com pelo menos 10 níveis

2.3 GERENCIADOR DE ARQUIVOS (`fm/`)
    - Listar diretórios (nome, tamanho, tipo)
    - Navegar com Enter, voltar com Backspace
    - Criar pasta, renomear, deletar (com confirmação)
    - Copiar, mover, colar
    - Abrir arquivo no editor ou paint conforme extensão
    - Barra de status com caminho atual e espaço livre

2.4 SHELL SIMPLES (`shell/`) — se não existir
    - Prompt, histórico (setas cima/baixo)
    - Comandos: ls, cd, pwd, cat, echo, mkdir, rm, cp, mv, clear, exit
    - Pipe simples e redirecionamento (se viável)

Para cada app: teste manual em QEMU, capture screenshot,
salve em ./evidencias/apps/<nome>.png.

============================================================
FASE 3 — INTEGRAÇÃO COM O DESKTOP
============================================================
1. Adicione os 3 apps ao menu do desktop.
2. Crie ícones (16x16 e 32x32) em ./assets/icons/.
3. Garanta que abrir/fechar cada app NÃO deixe processos zumbis.
4. Verifique se abrir os 3 apps simultaneamente funciona.
5. Meça uso de memória de cada app; documente em
   ./evidencias/apps/memoria.md.

============================================================
FASE 4 — CAÇA AO BUG `+0x68` (ULTRA POTÊNCIA)
============================================================
Objetivo: forçar o crash a acontecer de forma reproduzível.

4.1 Instrumentação:
   - Insira `ud2` ou `int3` imediatamente ANTES do `iretq` no trampolim.
   - Se o crash mudar de `+0x68` para o ponto de instrumentação,
     confirma que o problema é ANTES do iretq.
   - Se NÃO mudar, o problema é DEPOIS (user-code pós-iretq).

4.2 Stress test (rodar por no mínimo 2 horas):
   - Script `stress.sh` que:
     a) Abre e fecha o gerenciador de arquivos 1000x em loop
     b) Navega por diretórios com muitos arquivos
     c) Abre o editor, digita 10KB de texto, salva, fecha
     d) Abre o paint, desenha 100 formas, salva, fecha
     e) Tudo isso com `churn` de memória (malloc/free em loop paralelo)
   - Capture CADA crash com: RIP, RSP, RBP, registradores, stack dump,
     conteúdo do frame de retorno.
   - Salve em ./evidencias/crashes/crash-<timestamp>.log

4.3 Correlação:
   - Se crash sempre no mesmo RIP (+0x68), mapeie esse endereço
     usando o mapa do binário dinâmico carregado.
   - `readelf -l`, `objdump -d`, `nm` no binário suspeito.
   - Descubra QUAL função/instrução vive em +0x68.

4.4 Teste de hipótese final:
   - Se +0x68 é offset de binário dinâmico: investigue o loader.
   - Se +0x68 é offset de user-stack: investigue ABI do iretq.
   - Documente TUDO em ./evidencias/hipotese-final.md

============================================================
FASE 5 — MELHORIAS ESTRUTURAIS
============================================================
Com base na auditoria da Fase 1, implemente:
1. Guards de NULL em todos os pontos SUSPEITO/CRÍTICO.
2. Bounds checking em toda cópia de memória.
3. Comentários explicativos nos trechos asm do syscall trampolim.
4. Testes unitários para cada syscall (se framework existir).
5. Documentação em ./docs/ dos pontos críticos.

Se alguma melhoria for arriscada (pode quebrar algo),
NÃO implemente — apenas documente como recomendação.

============================================================
FASE 6 — RELATÓRIO FINAL
============================================================
Gere `~/relatorio-noturno-$(date +%Y%m%d).md` contendo:

1. Resumo executivo (1 parágrafo)
2. Estado do kernel: integridade, achados, selos por arquivo
3. Apps criados: status, screenshots, LOC, memória
4. Bug +0x68: status (resolvido? reproduzido? hipótese?)
5. Crashes capturados: quantidade, padrões, RIPs únicos
6. Melhorias implementadas vs recomendadas
7. Próximos passos sugeridos (máx. 5 itens)
8. Lista de TUDO que NÃO foi feito e por quê

Formato: Markdown, com tabelas, blocos de código e links
para os arquivos de evidência.

============================================================
REGRAS GERAIS
============================================================
- NUNCA deletar /tmp/bug2-rip-dump.log ou seus backups.
- NUNCA commitar sem antes rodar `make clean && make`.
- Se um teste travar por >10 min, mate e siga em frente.
- Se o QEMU travar, reinicie e continue de onde parou.
- Preserve evidências ANTES de tentar consertar qualquer coisa.
- Prefira deixar código funcionando a código bonito.
- Ao final, faça `git log --oneline > ./evidencias/git-log.txt`.
- Deixe o branch `noite/audit-*` pronto para revisão matinal.

Boa noite. Trabalhe.