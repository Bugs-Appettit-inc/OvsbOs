/* ♥ PROCESS ~ gerenciamento de processos lindo! */
/* ♥ PROCESS ~ Gerenciamento de processos fofinho!
 * Agora com scheduler round-robin! PID 0 = idle, PID 1+ = processos!
 * Estados: EMPTY, READY, RUNNING, BLOCKED, ZOMBIE~
 * Cada processo tem sua propria PML4 (isolamento de memoria)!
 * O timer IRQ0 chama schedule() pra dar vez pros outros~ kyun! */

#include "process.h"
#include "tss.h"
#include "memory.h"
#include "serial.h"
#include "console.h"
#include "syscall.h"

extern void fds_cleanup(void);

pcb_t pcb_table[MAX_PROC];
int current_pid = 0;
static int pcb_count = 0;
static int next_pid = 1;

extern void context_switch(pcb_t *current, pcb_t *next);
#include "user_prog_bin.h"

static void idle_entry(void) {
    serial_puts("[PROCESS] Idle rodando~ kyun!\r\n");
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static pcb_t *alloc_pcb(void) {
    if (pcb_count >= MAX_PROC) return 0;
    return &pcb_table[pcb_count++];
}

static int find_pid(int pid) {
    for (int i = 0; i < MAX_PROC; i++)
        if (pcb_table[i].pid == pid) return i;
    return -1;
}

void process_init(void) {
    serial_puts("[PROC] init~\r\n");
    fds_cleanup();
    for (int i = 0; i < MAX_PROC; i++) {
        for (int j = 0; j < (int)sizeof(pcb_t); j++)
            ((uint8_t *)&pcb_table[i])[j] = 0;
        pcb_table[i].pid = -1;
        pcb_table[i].state = PROC_EMPTY;
    }
    pcb_count = 0;
    current_pid = 0;
    next_pid = 1;

    /* Processo idle PID 0 */
    pcb_t *idle = alloc_pcb();
    if (!idle) return;

    void *kstack = kmalloc(STACK_SIZE);
    if (!kstack) return;

    idle->pid = 0;
    idle->state = PROC_READY;
    idle->is_user = 0;
    for (int i = 0; "idle"[i]; i++) idle->name[i] = "idle"[i];

    uint64_t *sp = (uint64_t *)((uint8_t *)kstack + STACK_SIZE);
    *--sp = (uint64_t)idle_entry;
    idle->kernel_rsp = (uint64_t)sp;
    idle->rsp0 = (uint64_t)sp;
    idle->pml4 = pml4_get_current();
    idle->parent_pid = 0;
    idle->exit_code = 0;
}

static void setup_user_stack(pcb_t *pcb, void *entry, void *ustack, uint64_t ustack_size) {
    uint64_t *sp = (uint64_t *)pcb->kernel_rsp;
    /* Empilha na ordem: SS, RSP, RFLAGS, CS, RIP, depois 16 regs */
    *--sp = 0x23;                          /* SS (user data) */
    *--sp = (uint64_t)ustack + ustack_size; /* RSP (topo da pilha user) */
    *--sp = 0x202;                          /* RFLAGS (IF=1) */
    *--sp = 0x1B;                           /* CS (user code 64-bit) */
    *--sp = (uint64_t)entry;                /* RIP (entry point) */
    /* 16 registradores (pop na ordem: r15,r14,...,rbp,rbx,r11,...,rax) */
    *--sp = 0; /* r15 */  *--sp = 0; /* r14 */
    *--sp = 0; /* r13 */  *--sp = 0; /* r12 */
    *--sp = 0; /* rbp */  *--sp = 0; /* rbx */
    *--sp = 0; /* r11 */  *--sp = 0; /* r10 */
    *--sp = 0; /* r9 */   *--sp = 0; /* r8 */
    *--sp = 0; /* rdi */  *--sp = 0; /* rsi */
    *--sp = 0; /* rdx */  *--sp = 0; /* rcx */
    *--sp = 0; /* rax */
    pcb->kernel_rsp = (uint64_t)sp;
}

/* ♥ process_create_user ~ API antiga, mantida pra compatibilidade!
 * Usa a PML4 atual (compartilhada) ~ sem isolamento total! */
int process_create_user(const char *name, void *entry, void *user_stack, uint64_t user_stack_size) {
    pcb_t *pcb = alloc_pcb();
    if (!pcb) return -1;

    void *kstack = kmalloc(STACK_SIZE);
    if (!kstack) return -1;

    pcb->pid = next_pid++;
    pcb->state = PROC_READY;
    pcb->is_user = 1;
    for (int i = 0; name[i] && i < PROC_NAME_MAX - 1; i++) pcb->name[i] = name[i];

    uint64_t *ksp = (uint64_t *)((uint8_t *)kstack + STACK_SIZE);
    pcb->kernel_rsp = (uint64_t)ksp;
    pcb->rsp0 = (uint64_t)ksp;
    pcb->pml4 = pml4_get_current();
    pcb->parent_pid = current_pid;
    pcb->exit_code = 0;
    pcb->program_break = 0x201000;
    pcb->heap_start = 0x201000;

    setup_user_stack(pcb, entry, user_stack, user_stack_size);
    
    /* Inicializa registradores salvos (senão o context_switch pega lixo!) */
    pcb->s_rbx = 0;
    pcb->s_rbp = 0;
    pcb->s_r12 = 0;
    pcb->s_r13 = 0;
    pcb->s_r14 = 0;
    pcb->s_r15 = 0;
    
    return pcb->pid;
}

/* ♥ proc_spawn ~ API nova do TipOS! Cria processo com PML4 propria!
 * entry = endereco virtual do codigo, user_stack_top = topo da pilha user */
int proc_spawn(const char *name, void *entry, void *user_stack_top) {
    int slot = -1;
    for (int i = 0; i < MAX_PROC; i++)
        if (pcb_table[i].state == PROC_EMPTY) { slot = i; break; }
    if (slot < 0) return -1;

    void *kstack = mmap_user(0, STACK_SIZE, 3, 0);
    if (!kstack) return -1;

    uint64_t my_pml4 = pml4_create();
    if (!my_pml4) { munmap_user(kstack, STACK_SIZE); return -1; }

    pcb_t *p = &pcb_table[slot];
    p->pid = next_pid++;
    p->state = PROC_READY;
    p->is_user = 1;

    int ni = 0;
    while (name[ni] && ni < PROC_NAME_MAX - 1) { p->name[ni] = name[ni]; ni++; }
    p->name[ni] = '\0';

    p->rsp0 = (uint64_t)kstack + STACK_SIZE;
    p->pml4 = my_pml4;
    p->parent_pid = current_pid;
    p->exit_code = 0;
    p->program_break = 0x201000;
    p->heap_start = 0x201000;

    uint64_t *sp = (uint64_t *)p->rsp0;
    *(--sp) = 0x23;
    *(--sp) = (uint64_t)user_stack_top;
    *(--sp) = 0x202;
    *(--sp) = 0x1B;
    *(--sp) = (uint64_t)entry;
    *(--sp) = 0; *(--sp) = 0; *(--sp) = 0; *(--sp) = 0;
    *(--sp) = 0; *(--sp) = 0; *(--sp) = 0; *(--sp) = 0;
    *(--sp) = 0; *(--sp) = 0; *(--sp) = 0; *(--sp) = 0;
    *(--sp) = 0; *(--sp) = 0;

    p->kernel_rsp = (uint64_t)sp;
    p->s_rbx = 0; p->s_rbp = 0; p->s_r12 = 0;
    p->s_r13 = 0; p->s_r14 = 0; p->s_r15 = 0;

    return p->pid;
}

/* ♥ process_switch_to ~ API antiga, compatibilidade */
void process_switch_to(int pid) {
    pcb_t *current = process_current();
    if (!current) return;

    pcb_t *next = 0;
    for (int i = 0; i < pcb_count; i++) {
        if (pcb_table[i].pid == pid) { next = &pcb_table[i]; break; }
    }
    if (!next) return;

    pcb_table[current_pid].state = PROC_READY;
    current_pid = pid;
    next->state = PROC_RUNNING;
    tss_set_rsp0(next->rsp0);
    context_switch(current, next);
}

/* ♥ process_exit_current ~ API antiga, usada pela syscall SYS_exit */
void process_exit_current(int code) {
    pcb_t *p = process_current();
    if (!p) return;
    fds_cleanup();
    p->exit_code = code;
    p->state = PROC_ZOMBIE;

    proc_wake_parent(p->pid);

    /* Volta pro pai */
    int parent = p->parent_pid;
    if (parent < 0 || parent >= MAX_PROC) parent = 0;
    if (pcb_table[parent].state == PROC_BLOCKED || pcb_table[parent].state == PROC_READY)
        pcb_table[parent].state = PROC_READY;
    current_pid = parent;
    pcb_t *next = &pcb_table[parent];
    tss_set_rsp0(next->rsp0);
    context_switch(p, next);
}

/* ♥ proc_exit ~ API nova do TipOS! Exit com schedule */
void proc_exit(int code) {
    if (current_pid < 0 || current_pid >= MAX_PROC) return;
    fds_cleanup();
    pcb_t *p = &pcb_table[current_pid];
    p->exit_code = code;
    p->state = PROC_ZOMBIE;
    proc_wake_parent(p->pid);
    schedule();
    for (;;) __asm__ volatile("hlt");
}

void proc_wake_parent(int child_pid) {
    for (int i = 0; i < MAX_PROC; i++) {
        if (pcb_table[i].state == PROC_BLOCKED) {
            for (int j = 0; j < MAX_PROC; j++) {
                if (pcb_table[j].pid == child_pid &&
                    pcb_table[j].parent_pid == pcb_table[i].pid) {
                    pcb_table[i].state = PROC_READY;
                    return;
                }
            }
        }
    }
}

int proc_waitpid(int pid, int *exit_code) {
    for (;;) {
        int found = 0;
        for (int i = 0; i < MAX_PROC; i++) {
            if (pcb_table[i].pid == pid) {
                found = 1;
                if (pcb_table[i].state == PROC_ZOMBIE) {
                    if (exit_code) *exit_code = pcb_table[i].exit_code;
                    munmap_user((void *)(pcb_table[i].rsp0 - STACK_SIZE), STACK_SIZE);
                    pml4_destroy(pcb_table[i].pml4);
                    pcb_table[i].state = PROC_EMPTY;
                    return pid;
                }
                pcb_table[current_pid].state = PROC_BLOCKED;
                schedule();
                break;
            }
        }
        if (!found) return -1;
    }
}

pcb_t *process_current(void) {
    for (int i = 0; i < pcb_count; i++)
        if (pcb_table[i].pid == current_pid) return &pcb_table[i];
    return 0;
}

int process_current_pid(void) {
    return current_pid;
}

static int find_next_ready(void) {
    int start = current_pid;
    for (int i = 0; i < MAX_PROC; i++) {
        int idx = (start + i) % MAX_PROC;
        if (pcb_table[idx].state == PROC_READY)
            return idx;
    }
    return current_pid; /* fica no mesmo se nao achar */
}

/* ♥ schedule ~ Round-robin scheduler chamado pelo timer! */
void schedule(void) {
    int next_idx = find_next_ready();
    if (next_idx == -1 || next_idx == current_pid) return;
    int prev = current_pid;

    if (pcb_table[prev].state == PROC_RUNNING)
        pcb_table[prev].state = PROC_READY;

    pcb_table[next_idx].state = PROC_RUNNING;
    current_pid = next_idx;
    tss_set_rsp0(pcb_table[next_idx].rsp0);
    context_switch(&pcb_table[prev], &pcb_table[next_idx]);
}

/* ♥ proc_yield ~ "Me da a vez, CPU! >_<" */
void proc_yield(void) {
    schedule();
}
