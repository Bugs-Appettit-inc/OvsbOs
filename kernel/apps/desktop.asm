; ♥ Desktop Ring 3 ~ Janela desenhada manualmente!
bits 64
org 0x200000

start:
    ; SYS_disp_get_fb (202)
    mov rax, 202
    lea rdi, [fb]
    lea rsi, [width]
    lea rdx, [height]
    mov r10, pitch_ptr
    xor r8, r8
    xor r9, r9
    int 0x80
    
    cmp qword [fb], 0
    je exit
    
    ; Fundo cinza escuro (desktop background)
    mov rdi, [fb]
    mov eax, [width]
    mul dword [height]
    mov rcx, rax
    mov eax, 0xFF2D2D3A
.fill_bg:
    mov [rdi], eax
    add rdi, 4
    dec rcx
    jnz .fill_bg
    
    ; === JANELA 1 ===
    call draw_window
    
    ; Flush
    mov rax, 203
    mov rdi, [fb]
    xor rsi, rsi
    xor rdx, rdx
    xor r10, r10
    xor r8, r8
    xor r9, r9
    int 0x80

exit:
    mov rax, 1
    xor rdi, rdi
    int 0x80

; ============================================
; draw_window - Desenha uma janela completa
; Posição: (100, 80), Tamanho: 400x300
; ============================================
draw_window:
    push rbp
    mov rbp, rsp
    
    mov rdi, [fb]
    mov r8d, [pitch]
    shr r8d, 2          ; r8 = pitch em pixels
    mov r9d, [width]
    
    ; Barra de título (y=80 a 108, altura 28)
    mov eax, 0xFF1A1A6E  ; cor da title bar
    mov r10d, 80          ; y
.title_y:
    cmp r10d, 108
    jge .body
    mov r11d, 100         ; x
.title_x:
    cmp r11d, 500
    jge .title_next
    ; fb[y*pitch + x] = color
    mov eax, r10d
    mul r8d
    add eax, r11d
    shl rax, 2
    add rax, rdi
    mov dword [rax], 0xFF1A1A6E
    inc r11d
    jmp .title_x
.title_next:
    inc r10d
    jmp .title_y
    
    ; Corpo da janela (y=108 a 380)
.body:
    mov r10d, 108
.body_y:
    cmp r10d, 380
    jge .borders
    mov r11d, 100
.body_x:
    cmp r11d, 500
    jge .body_next
    ; Verifica se é borda (2px)
    mov edx, r11d
    sub edx, 100
    cmp edx, 2
    jl .border_pixel
    mov edx, 500
    sub edx, r11d
    cmp edx, 2
    jle .border_pixel
    mov edx, r10d
    sub edx, 108
    cmp edx, 2
    jl .border_pixel
    mov edx, 380
    sub edx, r10d
    cmp edx, 2
    jle .border_pixel
    ; Interior
    mov eax, 0xFF3D3D5A
    jmp .draw_pixel
.border_pixel:
    mov eax, 0xFF5A5A8A
.draw_pixel:
    push rax
    mov eax, r10d
    mul r8d
    add eax, r11d
    shl rax, 2
    add rax, rdi
    pop rdx
    mov [rax], edx
    inc r11d
    jmp .body_x
.body_next:
    inc r10d
    jmp .body_y
    
.borders:
    pop rbp
    ret

section .data
fb:       dq 0
width:    dd 0
height:   dd 0
pitch:    dd 0
pitch_ptr: dq pitch
