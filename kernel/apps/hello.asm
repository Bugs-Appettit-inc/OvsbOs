; ♥ Hello Ring 3 ~ App de teste!
bits 64
org 0x200000

start:
    ; SYS_write (4) - "Hello!\n"
    mov rax, 4
    mov rdi, 1
    lea rsi, [msg]
    mov rdx, 7
    int 0x80
    
    ; SYS_exit (1)
    mov rax, 1
    xor rdi, rdi
    int 0x80

section .data
msg: db "Hello!", 10
