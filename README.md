# OvsbOS

OvsbOS é um projeto de sistema operacional e estação de trabalho focado em compatibilidade ampla, alto desempenho e experiência desktop moderna.

A ideia central é manter um ambiente que funcione em múltiplos sistemas e hardware, com foco em desempenho bruto, eficiência de execução e uma base técnica modular.

## Visão geral da stack

- OvsbOS: sistema operacional completo, com ambiente de desktop, runtime, aplicativos e camada de usuário
- OvsbK: kernel base do projeto e do OvsbOS
- Ovsb OWT: toolkit/front-end para facilitar a criação de interfaces e apps
- Ovsb WM: biblioteca de janelas e gerenciamento visual, vinculada ao OWT
- Ovsb SDK: conjunto de ferramentas para desenvolvimento de apps e componentes do sistema

## Filosofia do projeto

- Compatibilidade com vários sistemas e arquiteturas
- Ambiente de trabalho voltado para produtividade e performance
- Kernel enxuto, estável e modular
- UI e WM prontas para apps e ferramentas de desktop
- SDK que acelera o desenvolvimento de software nativo para o ecossistema Ovsb

## Estrutura atual

```text
OvsbOs/
├── kernel/            # OvsbK: núcleo do sistema
├── system/            # camada do sistema em ring 3
├── build/             # artefatos gerados
├── scripts/           # automação de setup e execução
├── iso/               # configuração de boot/GRUB
├── tests/             # testes e utilitários
├── docs/              # documentação adicional
├── README.md          # visão geral do projeto
├── Makefile           # orquestração raiz
├── setup.sh           # setup rápido na raiz
├── run.sh             # execução rápida na raiz
├── LICENSE
├── .gitignore
└── user_prog.asm      # programa de teste do kernel
```

## Componentes

### OvsbK

Responsável pelo core do sistema:
- boot
- memória
- processos
- drivers
- interrupções
- syscall
- infraestrutura do kernel em ring 0

### OvsbOS

Camada completa do ambiente operacional:
- runtime do usuário
- aplicações
- integração com o WM
- UI e desktop
- compatibilidade com uso real de estação de trabalho

### Ovsb OWT

Camada de front-end para facilitar criação de apps e interfaces:
- widgets
- layout
- componentes visuais
- abstração de UI para apps do sistema

### Ovsb WM

Biblioteca de janela e composição visual:
- janelas
- gerenciamento de focus
- eventos de interface
- integração com o OWT

### Ovsb SDK

Ferramenta de desenvolvimento para apps do ecossistema:
- build de apps
- helpers de desenvolvimento
- bibliotecas e recursos compartilhados
- base para criar software compatível com o sistema

## Build e execução

```bash
bash setup.sh
bash run.sh
```

Ou via Makefile:

```bash
make kernel
make system
make run
```

## Visão de futuro

O projeto foi pensado para evoluir em camadas independentes:

- OvsbK: kernel puro, base da plataforma
- OvsbOS: sistema completo em ambiente de execução
- Ovsb OWT: frontend para apps e UX
- Ovsb WM: camada gráfica e de janelas
- Ovsb SDK: ambiente de desenvolvimento para apps e ferramentas

Essa separação permite que cada parte cresça de forma organizada, mantendo o ecossistema coeso e escalável.

## Resumo

OvsbOS não é apenas um kernel; é uma plataforma de workstation moderna, com foco em:
- compatibilidade
- desempenho bruto
- desktop funcional
- apps nativas
- estrutura modular e escalável

---

O objetivo é construir uma base sólida para um sistema que seja poderoso, compatível e agradável de trabalhar.