; ♥ Desktop Ring 3 ~ Teste pixel no canto!
bits 64
org 0x200000

start:
    mov rax, 202
    lea rdi, [fb_addr]
    lea rsi, [width]
    lea rdx, [height]
    mov r10, pitch_ptr
    mov r8, 0
    mov r9, 0
    int 0x80
    
    ; Verifica se fb_addr é válido
    mov rax, [fb_addr]
    cmp rax, 0
    je exit
    
    ; Escreve "FB OK" via syscall
    mov rax, 4
    mov rdi, 1
    lea rsi, [msg_ok]
    mov rdx, 5
    int 0x80
    
    ; Pinta os primeiros 1000 pixels de BRANCO
    mov rdi, [fb_addr]
    mov rcx, 1000
    mov eax, 0xFFFFFFFF
.paint:
    mov [rdi], eax
    add rdi, 4
    dec rcx
    jnz .paint
    
    ; Flush
    mov rax, 203
    mov rdi, [fb_addr]
    xor rsi, rsi
    xor rdx, rdx
    xor r10, r10
    xor r8, r8
    xor r9, r9
    int 0x80
    
    ; Mensagem de saída
    mov rax, 4
    mov rdi, 1
    lea rsi, [msg_done]
    mov rdx, 4
    int 0x80

exit:
    mov rax, 1
    xor rdi, rdi
    int 0x80

section .data
fb_addr:  dq 0
width:    dd 0
height:   dd 0
pitch:    dd 0
pitch_ptr: dq pitch
msg_ok:   db "fb_ok"
msg_done: db "done"
