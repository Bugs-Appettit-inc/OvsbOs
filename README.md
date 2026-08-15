# OvsbOS

OvsbOS é um projeto pessoal de sistema operacional e ambiente de trabalho, com foco em compatibilidade multi-plataforma, desempenho bruto, ergonomia de desenvolvimento e uma base modular para criação de aplicações.

Não é a mesma coisa que o TIP. O objetivo aqui é manter a identidade do projeto separada:

- OvsbOS = sistema operacional completo e ambiente de trabalho
- OvsbK = kernel do TIP e do OvsbOS
- Ovsb OWT = framework/front-end para facilitar a criação de apps
- Ovsb WM = biblioteca de gerenciamento de janelas, integrada ao OWT
- Ovsb SDK = conjunto de ferramentas para desenvolvimento de apps e utilitários

## Visão geral

O projeto busca criar uma base sólida para:

- kernel em modo 64 bits
- ambiente gráfico e ferramentas de UI
- window manager e widgets reutilizáveis
- compatibilidade com diferentes sistemas e ambientes de execução
- infraestrutura de desenvolvimento leve e fácil de usar

## Filosofia do projeto

- compatibilidade entre Linux, WSL e Windows
- desenvolvimento simples para quem está iterando rápido
- manter o projeto funcional mesmo em fases iniciais de construção
- priorizar arquitetura limpa e extensível
- reduzir atrito na rotina de build/test/run

## Estrutura principal

- kernel/ — núcleo do sistema e drivers
- system/ — biblioteca do sistema, libc, apps e recursos do ambiente
- iso/ — estrutura de boot e configuração do GRUB
- build/ — artefatos gerados compilados
- tests/ — testes e utilitários de validação
- tools/ — ferramentas auxiliares
- dev.py — script principal de desenvolvimento e automação
- dev_gui.py — launcher em GUI para ações rápidas
- dependencies.sh — instalador de dependências para Linux/WSL
- dependencies.ps1 — instalador de dependências para Windows PowerShell

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

### GUI

```bash
python3 dev_gui.py
```

## Fluxo de desenvolvimento

### Ver menu

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

### Compilar

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

## Observações importantes

- O projeto depende de ferramentas como GCC, NASM, make, qemu-system-x86 e Python
- O launcher GUI tenta detectar automaticamente o sistema e o interpretador Python correto
- O ambiente Windows usa `py`/`python` quando necessário, em vez de assumir `python3`
- A execução do sistema em QEMU depende do caminho de boot correto via ISO/GRUB

## Status atual

O OvsbOS está em desenvolvimento ativo. A ideia é que ele funcione como base de um ambiente de trabalho moderno e leve, com potencial para evoluir em direção a um sistema operacional completo, mantendo modularidade e compatibilidade.

## Contribuição

Este projeto é pessoal e experimental, mas o foco é manter uma arquitetura clara, fácil de evoluir e compatível com múltiplos cenários de desenvolvimento.

---

OvsbOS não é apenas um Os qualquer; é a visão do sistema como um ambiente de trabalho inteiro.
