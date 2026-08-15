# Tabela Completa de Syscalls do OvsbOS

## Convenção de Chamada

- **Número da syscall**: RAX
- **Argumento 1**: RDI
- **Argumento 2**: RSI
- **Argumento 3**: RDX
- **Argumento 4**: R10
- **Retorno**: RAX (-1 em caso de erro)
- **Invocação**: `int 0x80`

---

## Syscalls por Categoria

### Processo (6 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 1 | `exit` | code (RDI) | N/A | Encerra o processo com código de saída |
| 20 | `getpid` | — | pid | Retorna o ID do processo atual |
| 24 | `getuid` | — | uid | Retorna o user ID real |
| 25 | `geteuid` | — | euid | Retorna o user ID efetivo |
| 47 | `getgid` | — | gid | Retorna o group ID real |
| 48 | `getegid` | — | egid | Retorna o group ID efetivo |

### Arquivo (10 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 3 | `read` | fd, buf, count | bytes_read | Lê até `count` bytes de `fd` para `buf` |
| 4 | `write` | fd, buf, count | bytes_written | Escreve até `count` bytes de `buf` para `fd` |
| 5 | `open` | path, flags | fd | Abre arquivo em `path` com `flags` |
| 6 | `close` | fd | 0/-1 | Fecha o arquivo descriptor |
| 10 | `unlink` | path | 0/-1 | Deleta o arquivo em `path` |
| 33 | `access` | path, mode | 0/-1 | Verifica permissão de acesso |
| 188 | `stat` | path, buf | 0/-1 | Preenche `buf` com info de arquivo |
| 189 | `fstat` | fd, buf | 0/-1 | Preenche `buf` com info de fd |
| 199 | `lstat` | path, buf | 0/-1 | Preenche `buf` com info de link simbólico |
| 200 | `lseek` | fd, offset, whence | pos | Posiciona fd em `offset` |

### Diretório (2 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 136 | `mkdir2` | path, mode | 0/-1 | Cria diretório em `path` |
| 137 | `rmdir2` | path | 0/-1 | Remove diretório vazio em `path` |

### Memória (3 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 197 | `mmap` | addr, size, prot, flags | addr | Mapeia `size` bytes de memória |
| 73 | `munmap` | addr, size | 0/-1 | Desmapeia `size` bytes de `addr` |
| 74 | `mprotect` | addr, size, prot | 0/-1 | Muda proteção de region de memória |

### Sinal (2 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 134 | `sigaction` | sig, action, old | 0/-1 | Define handler para sinal |
| 173 | `sigreturn` | — | — | Retorna de handler de sinal |

### Tempo (1 syscall)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 116 | `gettimeofday` | tv, tz | 0/-1 | Preenche `tv` com tempo atual |

### I/O e Teclado (2 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 54 | `ioctl` | fd, request, arg | result | Operação de controle de I/O |
| 198 | `kbhit` | — | 0/1 | Retorna 1 se há tecla disponível |

### Display e Window Manager (5 syscalls)

| Número | Nome | Argumentos | Retorno | Descrição |
|--------|------|-----------|---------|-----------|
| 202 | `disp_get_fb` | — | addr | Retorna endereço do framebuffer |
| 203 | `disp_flush` | — | 0/-1 | Libera/flush a display |
| 204 | `wm_get_backbuf` | — | addr | Obtém endereço do backbuffer do WM |
| 205 | `wm_get_info` | buf | 0/-1 | Preenche `buf` com info do WM |
| 206 | `wm_flush` | — | 0/-1 | Flush do window manager |

---

## Resumo de Números Livres

Syscalls 7-9, 11-19, 21-23, 26-32, 34-46, 49-53, 55-72, 75-115, 117-135, 138-172, 174-188, 190-196, 201 estão disponíveis para expansão.

---

## Flags de Open (O_*)

```c
#define O_RDONLY  0      /* Leitura apenas */
#define O_WRONLY  1      /* Escrita apenas */
#define O_RDWR    2      /* Leitura e escrita */
#define O_CREAT   0x200  /* Cria se não existir */
```

## Estrutura de Arquivo (stat)

```c
struct stat {
    uint32_t st_size;   /* Tamanho do arquivo */
};
```

## Estrutura de Tempo (timeval)

```c
struct timeval {
    uint64_t tv_sec;    /* Segundos */
    uint64_t tv_usec;   /* Microsegundos */
};
```

---

## Exemplos de Uso

### Ler um arquivo

```asm
mov rax, 5          ; SYS_open
mov rdi, path       ; path do arquivo
mov rsi, 0          ; flags O_RDONLY
int 0x80
mov r8, rax         ; salva fd em r8

mov rax, 3          ; SYS_read
mov rdi, r8         ; fd
mov rsi, buffer     ; buf
mov rdx, 256        ; count
int 0x80
```

### Escrever em stdout

```asm
mov rax, 4          ; SYS_write
mov rdi, 1          ; fd = stdout
mov rsi, message    ; buf
mov rdx, length     ; count
int 0x80
```

### Sair do processo

```asm
mov rax, 1          ; SYS_exit
mov rdi, 0          ; code
int 0x80
```

### Criar diretório

```asm
mov rax, 136        ; SYS_mkdir2
mov rdi, path       ; path
mov rsi, 0755       ; mode (octal)
int 0x80
```

---

## Observações

- Arquivo descriptors 0, 1, 2 são reservados para stdin, stdout, stderr
- Máximo de 16 descritores abertos por processo
- Chamadas erradas retornam -1 em RAX
- O estado das proteções de memória é preservado entre chamadas
- Window manager requer display já inicializada
