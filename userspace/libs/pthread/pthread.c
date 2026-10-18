/* pthread.c - implementação mínima de pthreads sobre o kernel OvsbMk
 *
 * Primitivos do kernel:
 *   syscall 56 = clone(CLONE_THREAD|CLONE_VM|...)  -> proc_clone_thread
 *   syscall 96 = futex(FUTEX_WAIT/FUTEX_WAKE)      -> sincronização
 *   syscall  1 = exit                               -> proc_thread_exit
 *   syscall 20 = getpid                             -> identificador de grupo
 *
 * Modelo de execução:
 *   clone() copia o frame do pai; a filha começa com RAX=0 e RSP = stack_top.
 *   fn/arg são passados através de slots indexados pelo endereço da pilha
 *   (salvos ANTES do clone, sem globais compartilhados -> sem corrida).
 */

#include "pthread.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_THREADS 16
#define STACK_SIZE  (64 * 1024)

typedef struct {
    uint64_t stack_addr;  /* chave: endereço do topo da pilha da thread */
    void *(*fn)(void *);
    void  *arg;
    op_pthread_t tid;
    int used;
} thread_slot_t;

static thread_slot_t thread_slots[MAX_THREADS];
static uint8_t thread_stacks[MAX_THREADS][STACK_SIZE];
static int nthreads = 0;

/* ---------- syscalls diretas ---------- */

static inline long sys_clone(uint64_t flags, uint64_t stack,
                             uint64_t ptid, uint64_t tls, uint64_t ctid) {
    long ret;
    __asm__ volatile (
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "mov %4, %%r10\n\t"
        "mov %5, %%r8\n\t"
        "syscall"
        : "=a"(ret)
        : "g"(flags), "g"(stack), "g"(ptid), "g"(tls), "g"(ctid), "a"(56)
        : "rcx", "r11", "memory", "rdi", "rsi", "rdx", "r10", "r8"
    );
    return ret;
}

static inline long sys_futex(volatile uint32_t *uaddr, int op, int val) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(96), "D"(uaddr), "S"(op), "d"(val)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline void sys_exit(int code) {
    __asm__ volatile (
        "syscall"
        :
        : "a"(1), "D"(code)
        : "rcx", "r11", "memory"
    );
    for (;;) __asm__ volatile ("hlt");
}

static inline int sys_getpid(void) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(20)
        : "rcx", "r11", "memory"
    );
    return (int)ret;
}

/* ---------- trampolino da thread ---------- */

static void op_pthread_thread_trampoline(void) __attribute__((noreturn));

static void op_pthread_thread_trampoline(void) {
    /* Cada thread começa com RSP = stack_top (definido no clone). Busca o
     * slot por esse endereço. Usa < (topo é exclusive upper bound) porque
     * RSP aponta para o primeiro byte livre acima da região usável. */
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));

    void *(*fn)(void *) = NULL;
    void  *arg = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_slots[i].used &&
            thread_slots[i].stack_addr == rsp) {
            fn = thread_slots[i].fn;
            arg = thread_slots[i].arg;
            break;
        }
    }
    if (!fn) sys_exit(1);

    void *retval = fn(arg);
    op_pthread_exit(retval);
    for (;;) __asm__ volatile ("hlt");
}

/* ---------- API op_pthread ---------- */

int op_pthread_create(op_pthread_t *thread_, const op_pthread_attr_t *attr_,
                   void *(*start_routine_)(void *), void *arg_) {
    (void)attr_;

    int idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!thread_slots[i].used) { idx = i; break; }
    }
    if (idx < 0) return -12; /* ENOMEM */

    uint64_t stack_top = (uint64_t)(thread_stacks[idx] + STACK_SIZE);

    /* ~~ Salva fn/arg ANTES do clone, indexados por stack_addr.
     * Assim a filha acha seus próprios argumentos sem ler globais
     * que o pai poderia sobrescrever (condicao de corrida). ~~ */
    thread_slots[idx].stack_addr = stack_top;
    thread_slots[idx].fn = start_routine_;
    thread_slots[idx].arg = arg_;
    thread_slots[idx].tid = 0;
    thread_slots[idx].used = 1;

    long child = sys_clone(
        0x00010100ULL | 0x00000100ULL | 0x00000200ULL | 0x00000400ULL |
        0x00000800ULL | 0x00080000ULL | 0x00100000ULL | 0x00200000ULL,
        stack_top,
        (uint64_t)(uintptr_t)&thread_slots[idx].tid,  /* ptid */
        0,                                            /* tls  */
        (uint64_t)(uintptr_t)&thread_slots[idx].tid); /* ctid */

    if (child == 0) {
        /* filha: continua no trampolino (RIP do clone aponta de volta aqui,
         * mas a filha executa o trampolino via op_pthread_thread_trampoline
         * chamada a partir deste mesmo ponto -- veja nota abaixo). */
        op_pthread_thread_trampoline();
        for (;;) __asm__ volatile ("hlt");
    }

    if (child < 0) {
        thread_slots[idx].used = 0;
        return (int)child;
    }

    thread_slots[idx].tid = (op_pthread_t)child;
    nthreads++;
    *thread_ = (op_pthread_t)child;
    return 0;
}

void op_pthread_exit(void *retval_) {
    (void)retval_;
    op_pthread_t self = op_pthread_self();
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_slots[i].used && thread_slots[i].tid == self) {
            thread_slots[i].used = 0;
            nthreads--;
            break;
        }
    }
    sys_exit(0);
}

int op_pthread_join(op_pthread_t thread_, void **retval_) {
    (void)retval_;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_slots[i].used && thread_slots[i].tid == thread_) {
            /* futex WAIT no tid: o kernel zera clear_tid na thread exit. */
            while (thread_slots[i].used) {
                sys_futex((volatile uint32_t *)&thread_slots[i].used, 0, 1);
            }
            return 0;
        }
    }
    return -3; /* ESRCH */
}

int op_pthread_mutex_init(op_pthread_mutex_t *mutex_, const void *attr_) {
    (void)attr_;
    if (!mutex_) return -22;
    mutex_->__lock = 0;
    return 0;
}

int op_pthread_mutex_destroy(op_pthread_mutex_t *mutex_) {
    (void)mutex_;
    return 0;
}

int op_pthread_mutex_lock(op_pthread_mutex_t *mutex_) {
    while (1) {
        if (__sync_bool_compare_and_swap(&mutex_->__lock, 0, 1))
            return 0;
        sys_futex((volatile uint32_t *)&mutex_->__lock, 0, 1);
    }
}

int op_pthread_mutex_unlock(op_pthread_mutex_t *mutex_) {
    mutex_->__lock = 0;
    sys_futex((volatile uint32_t *)&mutex_->__lock, 1, 1);
    return 0;
}

op_pthread_t op_pthread_self(void) {
    /* Filho: identifica pela pilha. Pai: retorna o PID do grupo (getpid). */
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_slots[i].used &&
            rsp >= (uint64_t)thread_stacks[i] &&
            rsp <  (uint64_t)(thread_stacks[i] + STACK_SIZE))
            return thread_slots[i].tid;
    }
    return (op_pthread_t)sys_getpid();
}
