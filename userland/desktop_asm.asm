; ♥ Desktop Ring 3 ~ Assembly puro, sem SSE, sem Mach-O!
; Desenha na tela usando syscalls direto
bits 64
org 0x200000

start:
    ; syscall SYS_disp_get_fb (202)
    ; args: &fb_addr, &width, &height, &pitch
    mov rax, 202
    lea rdi, [fb_addr]
    lea rsi, [width]
    lea rdx, [height]
    mov r10, pitch_ptr
    mov r8, 0
    mov r9, 0
    int 0x80
    
    ; Verifica retorno
    cmp rax, 0
    jl exit
    cmp qword [fb_addr], 0
    je exit
    
    ; Pinta a tela de azul escuro (0xFF1A1A2E)
    mov rdi, [fb_addr]      ; base do framebuffer
    mov eax, [width]
    mul dword [height]       ; rax = total pixels
    mov rcx, rax             ; contador
    mov eax, 0xFF1A1A2E     ; cor
.fill_loop:
    mov [rdi], eax
    add rdi, 4
    dec rcx
    jnz .fill_loop
    
    ; Desenha retângulo vermelho (200,300) - (400,600)
    mov rdi, [fb_addr]
    mov eax, [pitch]
    shr eax, 2               ; pitch em pixels
    mov r8d, eax             ; r8 = pitch_pixels
    
    mov r9d, 200             ; y = 200
.y_loop:
    cmp r9d, 400
    jge .done_rect
    
    mov r10d, 300            ; x = 300
.x_loop:
    cmp r10d, 600
    jge .next_y
    
    ; fb[y * pitch_pixels + x] = 0xFFFF0000
    mov eax, r9d
    mul r8d                  ; eax = y * pitch_pixels
    add eax, r10d            ; + x
    shl rax, 2               ; * 4 (bytes)
    add rax, rdi             ; + fb base
    mov dword [rax], 0xFFFF0000
    
    inc r10d
    jmp .x_loop
.next_y:
    inc r9d
    jmp .y_loop
.done_rect:
    
    ; syscall SYS_disp_flush (203)
    mov rax, 203
    mov rdi, [fb_addr]
    mov rsi, 0
    mov rdx, 0
    mov r10, 0
    mov r8, 0
    mov r9, 0
    int 0x80

exit:
    ; syscall SYS_exit (1)
    mov rax, 1
    xor rdi, rdi
    int 0x80

section .data
fb_addr:  dq 0
width:    dd 0
height:   dd 0
pitch:    dd 0
pitch_ptr: dq pitch
