/* pthread.c - implementação mínima de pthreads sobre o kernel OvsbMk
 *
 * Baseado nos primitivos já existentes no kernel:
 *   syscall 56 = clone(CLONE_THREAD|CLONE_VM|...)  -> proc_clone_thread
 *   syscall 96 = futex(FUTEX_WAIT/FUTEX_WAKE)      -> sincronização
 *   syscall  1 = exit                               -> proc_thread_exit (thread)
 *   syscall 20 = getpid                             -> op_pthread_self
 *
 * O trampolino: clone retorna RAX=0 na nova thread e RAX=tid no pai.
 * Ambas continuam no mesmo ponto após o syscall; o filho (RAX==0)
 * salta para op_pthread_thread_trampoline, que lê fn/arg de slots
 * globais indexados por tid.
 */

#include "pthread.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_THREADS 16
#define STACK_SIZE  (64 * 1024)

static void *(*thread_fn[MAX_THREADS])(void *);
static void  *thread_arg[MAX_THREADS];
static op_pthread_t thread_tids[MAX_THREADS];
static int thread_slot[MAX_THREADS];
static uint8_t thread_stacks[MAX_THREADS][STACK_SIZE];
static int nthreads = 0;

/* ---------- syscalls diretas (inline asm) ---------- */

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
    op_pthread_t self = op_pthread_self();
    int idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_tids[i] == self) { idx = i; break; }
    }
    if (idx < 0) sys_exit(1);
    void *(*fn)(void *) = thread_fn[idx];
    void *arg = thread_arg[idx];
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
        if (!thread_slot[i]) { idx = i; break; }
    }
    if (idx < 0) return -12; /* ENOMEM */

    uint64_t stack_top = (uint64_t)(thread_stacks[idx] + STACK_SIZE);

    long child = sys_clone(
        0x00010100ULL | 0x00000100ULL | 0x00000200ULL | 0x00000400ULL |
        0x00000800ULL | 0x00080000ULL | 0x00100000ULL | 0x00200000ULL,
        stack_top,
        (uint64_t)(uintptr_t)&thread_tids[idx],  /* ptid */
        0,                                        /* tls  */
        (uint64_t)(uintptr_t)&thread_tids[idx]);  /* ctid */

    if (child == 0) {
        /* nova thread */
        thread_fn[idx] = start_routine_;
        thread_arg[idx] = arg_;
        op_pthread_thread_trampoline();
        for (;;) __asm__ volatile ("hlt");
    }

    if (child < 0) return (int)child;

    thread_slot[idx] = 1;
    thread_tids[idx] = (op_pthread_t)child;
    nthreads++;
    *thread_ = (op_pthread_t)child;
    return 0;
}

void op_pthread_exit(void *retval_) {
    (void)retval_;
    op_pthread_t self = op_pthread_self();
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_tids[i] == self) {
            thread_slot[i] = 0;
            thread_tids[i] = 0;
            nthreads--;
            break;
        }
    }
    sys_exit(0);
}

int op_pthread_join(op_pthread_t thread_, void **retval_) {
    (void)retval_;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_tids[i] == thread_ && thread_slot[i]) {
            /* futex WAIT no clear_tid: o kernel zera clear_tid no exit */
            while (thread_slot[i]) {
                sys_futex((volatile uint32_t *)&thread_slot[i], 0, 1);
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
    /* Cada thread tem seu stack único em thread_stacks[]; o RSP de qualquer
     * ponto da thread cai dentro do range do próprio stack. Usamos isso como
     * identificador (getpid() nao serve: clone com CLONE_THREAD devolve o
     * mesmo PID para todas as threads do grupo). */
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_slot[i] &&
            rsp >= (uint64_t)thread_stacks[i] &&
            rsp <  (uint64_t)(thread_stacks[i] + STACK_SIZE))
            return thread_tids[i];
    }
    return 0;
}
