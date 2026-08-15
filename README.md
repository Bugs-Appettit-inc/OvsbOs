# OvsbOS

OvsbOS é um projeto pessoal de sistema operacional e ambiente de trabalho, com foco em compatibilidade entre vários sistemas, desempenho bruto, estabilidade e simplicidade de desenvolvimento.

A identidade do projeto é simples e direta:

- OvsbOS = sistema operacional principal e ambiente de trabalho
- Ovsb K = kernel do projeto, responsável pela base do sistema
- Ovsb OWT = front-end/framework para facilitar a criação de apps
- Ovsb WM = biblioteca de janelas, ligada ao OWT
- Ovsb SDK = conjunto de ferramentas para desenvolvimento de apps

## Visão geral

O projeto nasceu como uma base para um sistema operacional completo, com foco em ambiente de trabalho e produtividade real. A intenção é criar uma plataforma com:

- compatibilidade entre Linux, WSL e Windows
- arquitetura modular e extensível
- kernel funcional e estruturado
- interface e janelas reutilizáveis
- desenvolvimento de apps com menos atrito
- base para performance e ambiente desktop

## Filosofia do projeto

- compatibilidade real entre sistemas diferentes
- desenvolvimento simples e direto
- foco em performance e estabilidade
- arquitetura limpa, organizada e reutilizável
- ambiente de trabalho funcional, mesmo em fases iniciais

## Objetivo principal

O OvsbOS busca ser um sistema operacional e ambiente de trabalho moderno, com os componentes abaixo trabalhando como uma base coerente:

- kernel e boot
- gerenciador de janelas
- toolkit/UX para apps
- SDK para desenvolvimento
- compatibilidade com vários ambientes de execução

É um projeto pessoal, com objetivo técnico e de estudo, mas com visão clara de sistema completo.

## Estrutura do repositório

- kernel/ — núcleo do sistema, drivers e boot
- system/ — biblioteca do sistema, app base, UI e infraestrutura
- iso/ — estrutura de boot do sistema via GRUB
- build/ — artefatos gerados pela compilação
- tests/ — testes e validações
- tools/ — utilitários e scripts auxiliares
- dev.py — script principal de desenvolvimento
- dev_gui.py — launcher gráfico para build/test/run
- dependencies.sh — instalador de dependências para Linux/WSL
- dependencies.ps1 — instalador de dependências para Windows PowerShell
- user_prog.asm — exemplo de programa do usuário

## Componentes do ecossistema Ovsb

### Ovsb K
Base do kernel. É a camada mais baixa do sistema, responsável por boot, memória, interrupções, drivers e estrutura de execução.

### Ovsb OWT
Front-end/framework para facilitar a criação de aplicações. Serve como base leve para construir UIs e apps mais rapidamente.

### Ovsb WM
Biblioteca para gerenciamento de janelas, integrada ao OWT. Controla a organização visual, layout e interação das janelas no ambiente.

### Ovsb SDK
Ferramenta de desenvolvimento de apps. Centraliza utilitários para criar, testar e compilar software para o ambiente do projeto.

## Compatibilidade

A arquitetura do projeto foi pensada para funcionar em vários ambientes de desenvolvimento, com foco em:

- Linux
- WSL
- Windows

Foi criado um conjunto de scripts e ferramentas para reduzir a fricção do setup e tornar o fluxo de desenvolvimento mais simples.

## Como começar

### Linux / WSL

```bash
bash ./dependencies.sh
python3 dev.py menu
```

### Windows (PowerShell)

```powershell
powershell -ExecutionPolicy Bypass -File .\dependencies.ps1
python .\dev.py menu
```

### GUI do launcher

```bash
python3 dev_gui.py
```

Caso o Windows não tenha `python3` disponível, o sistema identifica automaticamente `py`, `python` ou `python3` e usa o comando correto.

## Fluxo de desenvolvimento

### Menu interativo

```bash
python3 dev.py menu
```

### Instalar dependências

```bash
python3 dev.py install
```

### Testar ambiente

```bash
python3 dev.py test
```

### Compilar tudo

```bash
python3 dev.py build
```

### Executar

```bash
python3 dev.py run
```

### Fluxo completo

```bash
python3 dev.py all
```

## Dependências principais

As ferramentas esperadas incluem:

- gcc
- make
- nasm
- git
- python3
- qemu-system-x86
- tkinter

O instalador automático tenta verificar e provisionar esses itens conforme o ambiente em que o projeto está sendo executado.

## Build e execução

O projeto usa uma estrutura de build e boot orientada ao repositório, com GRUB e ISO para boot. O fluxo de execução foi centralizado para evitar scripts duplicados e confusão durante o desenvolvimento.

### Fluxo recomendado

1. instalar dependências
2. validar ambiente
3. compilar tudo
4. abrir o sistema via QEMU ou executar em modo manual

## Status atual

O OvsbOS está em desenvolvimento ativo, com foco em:

- estabilizar a base do kernel
- melhorar compatibilidade entre sistemas
- organizar a camada de UI e widgets
- manter um fluxo de desenvolvimento simples e rápido

Ainda é um projeto em evolução, mas com base clara, arquitetura definida e visão concreta de ambiente de trabalho e sistema operacional.

## Syscalls

O kernel oferece um conjunto de **31 syscalls** para aplicações em ring 3. Todas as chamadas são feitas via `int 0x80` com o número em RAX e argumentos em RDI, RSI, RDX, R10.

**Para documentação completa, exemplos de uso e tabelas detalhadas, veja [SYSCALLS.md](SYSCALLS.md)**

### Referência Rápida

| Syscall | Número | Descrição |
|---------|--------|-----------|
| exit | 1 | Encerra o processo |
| read | 3 | Lê dados de um arquivo descriptor |
| write | 4 | Escreve dados em um arquivo descriptor |
| open | 5 | Abre um arquivo |
| close | 6 | Fecha um arquivo descriptor |
| unlink | 10 | Deleta um arquivo |
| getpid | 20 | Obtém o ID do processo atual |
| getuid | 24 | Obtém o user ID |
| geteuid | 25 | Obtém o user ID efetivo |
| access | 33 | Verifica permissão de acesso a arquivo |
| getgid | 47 | Obtém o group ID |
| getegid | 48 | Obtém o group ID efetivo |
| ioctl | 54 | Operação de controle I/O |
| gettimeofday | 116 | Obtém tempo atual (seconds + microseconds) |
| sigaction | 134 | Define ação para um sinal |
| mkdir2 | 136 | Cria um diretório |
| rmdir2 | 137 | Remove um diretório |
| sigreturn | 173 | Retorna de handler de sinal |
| stat | 188 | Obtém informações de arquivo |
| fstat | 189 | Obtém informações de arquivo descriptor |
| lseek | 200 | Posiciona arquivo descriptor |
| kbhit | 198 | Verifica se tem tecla disponível |
| mmap | 197 | Mapeia region de memória |
| munmap | 73 | Desmapeia region de memória |
| mprotect | 74 | Muda proteção de region de memória |
| lstat | 199 | Obtém informações de link simbólico |
| disp_get_fb | 202 | Obtém framebuffer da display |
| disp_flush | 203 | Libera/flush da display |
| wm_get_backbuf | 204 | Obtém backbuffer do window manager |
| wm_get_info | 205 | Obtém informações do window manager |
| wm_flush | 206 | Flush do window manager |

## Observações finais

O OvsbOS é um projeto pessoal de sistema operacional, pensado para ser compatível com vários ambientes e forte em performance, base e ergonomia de desenvolvimento.

O conjunto de kernel, front-end, janelas e SDK forma uma base coerente para evoluir em direção a um ambiente funcional e completo.

---

OvsbOS é a minha base de sistema operacional e ambiente de trabalho, com kernel, front-end, gerenciador de janelas e SDK trabalhando juntos como um mesmo ecossistema.
