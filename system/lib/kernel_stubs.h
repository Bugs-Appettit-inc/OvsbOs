/* ♥ Kernel stubs para ring 3 ~ adapta funções do kernel! */
#ifndef KERNEL_STUBS_H
#define KERNEL_STUBS_H

#include <stdint.h>
#include <stddef.h>

/* Syscalls */
static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8  asm("r8")  = a5;
    register long r9  asm("r9")  = a6;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "memory", "r11", "rcx");
    return ret;
}

#define SYS_wm_get_backbuf 204
#define SYS_wm_get_info    205
#define SYS_wm_flush       206
#define SYS_mmap           197
#define SYS_munmap         73

/* kmalloc → mmap */
static inline void *kmalloc(size_t size) {
    return (void *)syscall6(SYS_mmap, 0, size, 3, 0, 0, 0);
}

/* kfree → munmap */
static inline void kfree(void *ptr) {
    syscall6(SYS_munmap, (long)ptr, 0, 0, 0, 0, 0);
}

/* console_printf → printf */
#define console_printf printf

/* Framebuffer global */
static uint32_t *g_fb = 0;
static int g_fb_w = 0, g_fb_h = 0;

static inline void fb_init(void) {
    syscall6(SYS_wm_get_backbuf, (long)&g_fb, 0, 0, 0, 0, 0);
    syscall6(SYS_wm_get_info, (long)&g_fb_w, (long)&g_fb_h, 0, 0, 0, 0);
}

#endif
