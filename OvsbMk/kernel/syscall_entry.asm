; moe moe kyun <3
; syscall_entry.asm — handler da instrução 'syscall' (Linux x86_64 ABI)
;
; Configurado via MSR LSTAR. Retorna pro userspace via IRETQ (sysretq
; não fecha com a GDT do TipOS — ver nota no fim do arquivo)~
; STAR configurado: SYSCALL_CS=0x08 (entrada), SYSRET_CS=0x18 (não usado)
;
; Convenção Linux x86_64 syscall:
;   rax = syscall number
;   rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5, r9 = arg6
;   rcx = return RIP (salvo pela CPU), r11 = return RFLAGS (salvo pela CPU)
;   rsp = user stack pointer
;
; Clobbers: rcx, r11 (salvos pela CPU em return RIP/RFLAGS)

bits 64

section .data
global current_rsp0
current_rsp0: dq 0          ; kernel stack top do processo atual
                             ; setado pelo context_switch

extern current_pid

section .text
global syscall_entry
extern syscall_handler_zig
extern frame_rb
extern frame_w
extern current_pid


syscall_entry:
    ; rcx = return RIP do userspace (syscall clobbered)
    ; r11 = return RFLAGS do userspace (syscall clobbered)
    ; rsp = user stack pointer
    ; rax = syscall number
    ; rdi = arg1, rsi = arg2, rdx = arg3, r10 = arg4, r8 = arg5, r9 = arg6
    ; Clobbers: rcx, r11 (salvos pela CPU em return RIP/RFLAGS)

    ; ~~ FASE1-redondo (+0x68): desliga IRQs para TODA a syscall.
    ; O epilogo monta o frame do iretq no stack do usuario (linhas 136-142)
    ; com rsp=usuario ainda em ring 0; uma irq0 (PIT) caindo nessa janela
    ; sobrescreve o slot RIP do frame com o endereco de .text da propria
    ; instrucao interrompida e o iretq devolve pro kernel em ring 3
    ; (PF err=0x15 em endereco do .text, ex.: 0x08028239/0x080285F8).
    ; FMASK ja limpa IF na entrada; o cli garante a invariante mesmo se
    ; algo mais abaixo houver sti. O iretq restaura os flags do usuario.
    cli

    ; Troca stack: salva user RSP em current_rsp0, carrega kernel stack
    xchg rsp, [current_rsp0]    ; rsp=kernel_stack, mem=user_rsp
                                 ; IF já tá limpo pelo FMASK, sem IRQ aninhada~ ☆

    ; ~~ Salva return RIP/RFLAGS fora do frame de 15 regs ~~
    ; O handler (syscall_linux.zig) sobrescreve o slot rcx do frame
    ; (regs[1]) com o arg4 (r10 → rcx p/ syscalls Linux traduzidas)~
    ; Se a gente restaurasse rcx do frame, o sysret voltaria pro arg4
    ; (o XORG caiu em 0x22 = flags do mmap! rssrsrs)~
    ; Guardamos return RIP/RFLAGS num local dedicado do kernel stack:
    sub rsp, 16
    mov [rsp], rcx              ; return RIP original
    mov [rsp + 8], r11          ; return RFLAGS original

    ; ~~ frame_trace tag=1 (save): frame de usuario antes do frame C ~~
    ; rsp = KSP0-16 (apos o sub). cli ativo: nao ha irq entre aqui e o iretq.
    mov r8d, dword [current_pid]   ; pid (clobbera r8; nao precisamos dele)
    push rax
    push rdx
    push r9
    push r10
    push r11                       ; r11 = user RFLAGS de return (preservado na pilha)
    rdtsc
    shl rdx, 32
    or rax, rdx                    ; rax = tsc
    mov r9, [frame_w]
    inc qword [frame_w]
    and r9, 31
    mov r10, frame_rb
    shl r9, 6
    lea r10, [r10 + r9]
    mov [r10 + 0], rax             ; tsc
    mov [r10 + 8], r8d             ; pid
    mov dword [r10 + 12], 1        ; tag = 1 (save)
    mov [r10 + 16], rcx            ; f0 user RIP de return (registrador, da CPU no syscall)
    mov [r10 + 24], r11            ; f1 user RFLAGS de return (registrador, da CPU no syscall)
    mov r9, [current_rsp0]         ; user rsp apos o xchg
    mov [r10 + 32], r9             ; f2 user rsp
    mov [r10 + 40], rsp            ; f3 kernel rsp neste ponto
    mov [r10 + 48], rcx            ; f4 return RIP (espelho do slot dedicado)
    mov [r10 + 56], r11            ; f5 return RFLAGS (espelho)
    pop r11
    pop r10
    pop r9
    pop rdx
    pop rax

    ; Salva registradores que a C espera preservados (callee-saved)
    ; + registradores voláteis que precisamos passar pro handler
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    push r11                    ; RFLAGS do userspace (salvo pela CPU)
    push r10                    ; arg4 original
    push r9                     ; arg6
    push r8                     ; arg5
    push rdi                    ; arg1
    push rsi                    ; arg2
    push rdx                    ; arg3
    ; Slot rcx do frame = ARG4 (r10), NUNCA o RIP de retorno: a CPU
    ; clobberou rcx com o return RIP, e o handler C (via Zig) lê o
    ; arg4 exatamente deste slot — inclusive p/ números TipOS nativos
    ; (o fixup r10→rcx do Zig pulava os nativos e o kernel escrevia em
    ; endereços selvagens, ex. PF@0). ABI Linux declara rcx CLOBBERED,
    ; então nenhum userspace pode depender do valor restaurado; o
    ; return RIP de verdade vive no save dedicado [rsp]/[rsp+8].
    push r10                    ; arg4 (era: push rcx = return RIP)
    push rax                    ; syscall number

    ; ~~ RBX = ponteiro do frame de 15 regs (igual idt.asm faz pro
    ; int 0x80)! Se essa syscall BLOQUEAR (wait4/read pipe), o
    ; context_switch salva KRNL_RSP=RBX — sem isso o processo
    ; acordava com pilha de kernel LIXO e corrompia tudo~ rssrsrs
    mov rbx, rsp

    mov rdi, rsp                ; rdi = struct registers* pra C
    call syscall_handler_zig

    ; Restaura registradores (ordem inversa)
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11                     ; restaura RFLAGS do userspace
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15

    ; ~~ frame_trace tag=2 (restore): antes de montar o frame do iretq ~~
    ; rsp = U (stack do usuario). Os slots do iretq em [U-40..U-8] ainda
    ; nao foram escritos pelas linhas abaixo; lê-los aqui (antes dos pushes,
    ; cli ativo => nao ha irq que os mude) mostra o que o iretq IRA ler.
    mov r8d, dword [current_pid]   ; pid
    mov r9, [rsp - 40]             ; f3 = conteudo atual do slot RIP do user
    mov r10, [rsp - 32]            ; f4 = conteudo atual do slot CS
    mov r11, [rsp - 24]            ; f5 = conteudo atual do slot RFLAGS
    push rax
    push rdx
    push r9
    push r10
    push r11
    rdtsc
    shl rdx, 32
    or rax, rdx                    ; rax = tsc
    mov r9, [frame_w]
    inc qword [frame_w]
    and r9, 31
    mov r10, frame_rb
    shl r9, 6
    lea r10, [r10 + r9]
    mov [r10 + 0], rax             ; tsc
    mov [r10 + 8], r8d             ; pid
    mov dword [r10 + 12], 2        ; tag = 2 (restore)
    mov [r10 + 16], rcx            ; f0 user RIP de return (registrador arg4 aqui; ver tag=1 para o alvo)
    mov [r10 + 24], r11            ; f1 user RFLAGS de return (registrador)
    mov [r10 + 32], rsp            ; f2 user rsp
    mov [r10 + 40], r9             ; f3 slot RIP (valor do stack do usuario antes da escrita)
    mov [r10 + 48], r10            ; f4 slot CS (valor do stack do usuario)
    mov [r10 + 56], r11            ; f5 slot RFLAGS (valor do stack do usuario)
    pop r11
    pop r10
    pop r9
    pop rdx
    pop rax

    ; ~~ Restaura return RIP/RFLAGS originais (ignora rcx do frame) ~~
    ; O slot rcx do frame carrega o ARG4 (r10) por construção — nunca o
    ; return RIP. Aqui pegamos o valor original salvo ANTES do frame~
    ; (tipo guardar a senha num cofre antes de sair de casa~)
    mov r11, [rsp + 8]          ; return RFLAGS original
    mov rcx, [rsp]              ; return RIP original
    add rsp, 16                 ; rsp volta pro topo do kernel stack

    ; Restaura current_rsp0 pro kernel stack (pra próxima syscall)
    ; E restaura o user RSP que o xchg salvou na variável~
    ; xchg de novo: troca rsp (kernel top) com [current_rsp0] (user RSP)
    ; numa instrução só, SEM clobberar RAX (return value do syscall)!
    ; (o return do mmap virou o RSP uma vez e o musl surtou~ rssrsrs)
    xchg rsp, [current_rsp0]    ; rsp = user RSP; current_rsp0 = kernel top ✓

    ; ~~ Retorno pro userspace: IRETQ (sysretq é furada com essa GDT!) ~~
    ; O HARDWARE (Intel SDM, SYSRET; AMD APM Vol.2 5.9.3) faz no retorno
    ; 64-bit (REX.W): CS = IA32_STAR[63:48]+16 e SS = IA32_STAR[63:48]+8.
    ; Não é bug do QEMU — ele segue o spec certinho! (documentado~ ☆)
    ; Com STAR[63:48]=0x18: sysretq daria CS=0x28 (slot do TSS!) e o iretq
    ; do próximo IRQ validaria CS=0x28 no GDT → #GP err=0x28 (vimos isso)~
    ; E o layout pro sysretq (user data em X+8, user code em X+16) não
    ; fecha com a GDT do TipOS (code 0x18, data 0x20 — invertido)~ >_<
    ; IRETQ carrega CS/SS do frame (0x1B/0x23) validados no GDT de
    ; verdade — igual o context_switch faz. SEMPRE iretq! ~ ☆
    ; Frame: RIP CS RFLAGS RSP SS (5 qwords) montado no stack do user~
    ; (rsp = user RSP original aqui → campo RSP = rsp exato, sem conta!)
    ; NOTA DE AUDITORIA (noite/audit): as 6 instrucoes abaixo sao o ponto
    ; cego entre o epilogo C e o iretq. Ordem importa: as 5 escritas vao
    ; PARA BAIXO do rsp (red-zone do usuario) e so o `sub` final "publica"
    ; o frame. IRQ/NMI chaveiam p/ kernel-stack via TSS (nao clobberam),
    ; mas NAO ha validacao de que o user-RSP e canonico/user antes das
    ; escritas — RSP selvagem = #PF em CPL0 em vez de PF-KILL limpo.
    ; Se precisar mexer aqui, releia evidencias/hipotese-final.md antes.
    mov [rsp - 40], rcx         ; (1) RIP (return original, não o arg4!)
    mov qword [rsp - 32], 0x1B  ; CS (user code ring 3 — GDT 0x18)
    mov [rsp - 24], r11         ; RFLAGS (return original)
    mov [rsp - 16], rsp         ; RSP (user RSP original)
    mov qword [rsp - 8], 0x23   ; SS (user data ring 3 — GDT 0x20)
    sub rsp, 40
    iretq

