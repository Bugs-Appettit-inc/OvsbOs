; ♥ Debug Ring 3 ~ O menor programa possível!
bits 64
org 0x200000

start:
    ; Só sai imediatamente
    mov rax, 1    ; SYS_exit
    xor rdi, rdi
    int 0x80
