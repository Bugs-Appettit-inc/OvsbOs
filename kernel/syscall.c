/* ♥ SYSCALL ~ 24 syscalls! kernel faz tudo pra vc~ */
/* ♥ SYSCALL ~ Chamadas de sistema! "Pede pro kernel fazer~ hihi!"
 * Agora com 24 syscalls! open, close, stat, lseek, mmap... tudo!
 * Argumentos: RAX=numero, RDI=a1, RSI=a2, RDX=a3, R10=a4~
 * Retorno vai em R15 (regs[14])~ Mesugaki mode ativado! kyun~ */

#include "syscall.h"
#include "process.h"
#include "memory.h"
#include "console.h"
#include "serial.h"
#include "pit.h"
#include "drivers/keyboard.h"
#include "fs/fat32.h"
#include "utils.h"
#include "gui/vesa.h"
#include "wm/wm.h"

/* Tabela de descritores de arquivo */
#define MAX_FDS 16
static struct {
    int used;
    char name[256];
    uint32_t pos;
} fds[MAX_FDS];

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x200

struct timeval { uint64_t tv_sec; uint64_t tv_usec; };
struct stat   { uint32_t st_size; };

void syscall_init(void) {
    for (int i = 0; i < MAX_FDS; i++) fds[i].used = 0;
}

void fds_cleanup(void) {
    for (int i = 3; i < MAX_FDS; i++) fds[i].used = 0;
}

static int alloc_fd(void) {
    for (int i = 3; i < MAX_FDS; i++) if (!fds[i].used) return i;
    return -1;
}

static void close_fd(int fd) {
    if (fd >= 3 && fd < MAX_FDS) fds[fd].used = 0;
}

static int str_equal(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

void syscall_handler(uint64_t *regs) {
    /* Salva e limpa CR0/CR4 para evitar #NM */
    uint64_t saved_cr0, saved_cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(saved_cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(saved_cr4));
    __asm__ volatile("clts");  /* limpa TS */
    {
        uint64_t cr0;
        __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~((uint64_t)0x04);  /* limpa EM */
        __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
    }
    {
        uint64_t cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= 0x200;  /* seta OSFXSR */
        __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    }
    uint64_t num  = regs[0];
    uint64_t a1 = regs[4];
    uint64_t a2 = regs[3];
    uint64_t a3 = regs[2];
    uint64_t a4 = regs[1];
    uint64_t ret = -1;

    switch (num) {
    case SYS_exit:
        process_exit_current((int)a1);
        for (;;) __asm__ volatile("hlt");
        break;

    case SYS_write: {
        int fd = (int)a1;
        const char *buf = (const char *)a2;
        int count = (int)a3;
        if (fd >= 3 && fd < MAX_FDS && fds[fd].used) {
            int r = fat32_write_file(fds[fd].name, (const uint8_t *)buf, count);
            ret = (r >= 0) ? count : -1;
            break;
        }
        if (fd == 1 || fd == 2) {
            for (int i = 0; i < count; i++) console_putchar(buf[i]);
            ret = count;
            break;
        }
        ret = -1;
        break;
    }

    case SYS_read: {
        int fd = (int)a1;
        char *buf = (char *)a2;
        int count = (int)a3;
        if (fd == 0 && buf && count > 0) {
            int i;
            for (i = 0; i < count; i++) {
                while (!keyboard_avail()) {
                    for (volatile int j = 0; j < 100; j++);
                }
                buf[i] = keyboard_read();
                if (buf[i] == '\n') { i++; break; }
                if (buf[i] == 3) break; /* ^C */
            }
            ret = i;
            break;
        }
        if (fd >= 3 && fd < MAX_FDS && fds[fd].used) {
            static uint8_t tmp[4096];
            int r = fat32_read_file(fds[fd].name, tmp, 4096);
            if (r < 0) { ret = -1; break; }
            int off = fds[fd].pos;
            int n = (count < r - off) ? count : (r - off);
            if (n < 0) { ret = 0; break; }
            for (int i = 0; i < n; i++) buf[i] = tmp[off + i];
            fds[fd].pos += n;
            ret = n;
            break;
        }
        ret = -1;
        break;
    }

    case SYS_open: {
        const char *path = (const char *)a1;
        if (str_equal(path, "/dev/tty") || str_equal(path, "/dev/stdin")) {
            ret = 0;
            break;
        }
        if (str_equal(path, "/dev/stdout")) { ret = 1; break; }
        if (str_equal(path, "/dev/stderr")) { ret = 2; break; }
        int fd = alloc_fd();
        if (fd < 0) { ret = -1; break; }
        int i = 0;
        while (path[i] && i < 255) { fds[fd].name[i] = path[i]; i++; }
        fds[fd].name[i] = '\0';
        fds[fd].pos = 0;
        fds[fd].used = 1;
        ret = fd;
        break;
    }

    case SYS_close:
        close_fd((int)a1);
        ret = 0;
        break;

    case SYS_access: {
        const char *path = (const char *)a1;
        if (str_equal(path, "/dev/tty") ||
            str_equal(path, "/dev/stdin") ||
            str_equal(path, "/dev/stdout") ||
            str_equal(path, "/dev/stderr")) { ret = 0; break; }
        ret = 0;
        break;
    }

    case SYS_fstat: {
        struct stat *st = (struct stat *)a2;
        if (!st) { ret = -1; break; }
        int fd = (int)a1;
        if (fd >= 3 && fd < MAX_FDS && fds[fd].used) {
            uint32_t size; uint8_t attr;
            if (fat32_stat(fds[fd].name, &size, &attr, NULL, NULL) == 0) {
                st->st_size = size; ret = 0; break;
            }
        }
        if (fd >= 0 && fd <= 2) { st->st_size = 0; ret = 0; break; }
        ret = -1;
        break;
    }

    case SYS_lstat:
    case SYS_stat: {
        const char *path = (const char *)a1;
        struct stat *st = (struct stat *)a2;
        if (!path || !st) { ret = -1; break; }
        uint32_t size; uint8_t attr;
        if (fat32_stat(path, &size, &attr, NULL, NULL) == 0) {
            st->st_size = size; ret = 0; break;
        }
        ret = -1;
        break;
    }

    case SYS_lseek: {
        int fd = (int)a1;
        int off = (int)a2;
        int whence = (int)a3;
        if (fd < 3 || fd >= MAX_FDS || !fds[fd].used) { ret = -1; break; }
        switch (whence) {
            case 0: fds[fd].pos = off; break;
            case 1: fds[fd].pos += off; break;
            case 2: {
            serial_puts("[syscall] write chamado\r\n");
                uint32_t size = 0;
                uint8_t attr;
                if (fat32_stat(fds[fd].name, &size, &attr, NULL, NULL) == 0)
                    fds[fd].pos = size + off;
                break;
            }
            default: ret = -1; break;
        }
        ret = fds[fd].pos;
        break;
    }

    case SYS_unlink:
        if (fat32_delete_file((const char *)a1) == 0) ret = 0;
        break;

    case SYS_mkdir2:
        if (fat32_mkdir((const char *)a1) == 0) ret = 0;
        break;

    case SYS_rmdir2:
        if (fat32_rmdir((const char *)a1) == 0) ret = 0;
        break;

    case SYS_mmap:
        ret = (uint64_t)mmap_user((void *)a1, a2, (int)a3, (int)a4);
        break;

    case SYS_munmap:
        ret = munmap_user((void *)a1, a2);
        break;

    case SYS_mprotect:
        ret = 0;
        break;

    case SYS_kbhit:
        ret = keyboard_avail();
        break;

    case SYS_getpid:
        ret = process_current_pid();
        break;

    case SYS_getuid:
    case SYS_geteuid:
    case SYS_getgid:
    case SYS_getegid:
        ret = 0;
        break;

    case SYS_ioctl:
        ret = 0;
        break;

    case SYS_sigaction:
    case SYS_sigreturn:
        ret = 0;
        break;

    case SYS_gettimeofday: {
        struct timeval *tv = (struct timeval *)a1;
        if (tv) { tv->tv_sec = timer_ticks / 100; tv->tv_usec = 0; }
        ret = 0;
        break;
    }

    /* ♥ Display API ~ mapeia framebuffer pro user space */
    case SYS_disp_get_fb: {
        extern int g_fb_active;
        extern framebuffer_t g_fb;
        if (!g_fb_active || g_fb.bpp != 32) { ret = -1; break; }
        uint64_t user_fb_va = 0xFFFFFFFF80000000ULL;
        uint64_t fb_size = (uint64_t)g_fb.pitch * g_fb.height;
        fb_size = (fb_size + 0x1FFFFF) & ~0x1FFFFF;
        pcb_t *cur = process_current();
        int r = pml4_map_phys(cur->pml4, user_fb_va, g_fb.addr, fb_size, 1);
        if (r < 0) { ret = -1; break; }
        uint64_t *addr   = (uint64_t *)a1;
        uint32_t *width  = (uint32_t *)a2;
        uint32_t *height = (uint32_t *)a3;
        uint32_t *pitch  = (uint32_t *)a4;
        if (addr)   *addr   = user_fb_va;
        if (width)  *width  = g_fb.width;
        if (height) *height = g_fb.height;
        if (pitch)  *pitch  = g_fb.pitch;
        ret = 0;
        break;
    }

    case SYS_disp_flush: {
        extern int g_fb_active;
        extern framebuffer_t g_fb;
        void *backbuffer = (void *)a1;
        if (!g_fb_active || !backbuffer) break;
        uint32_t *fb_phys = (uint32_t *)(uintptr_t)g_fb.addr;
        size_t pixels = (size_t)g_fb.pitch * g_fb.height / 4;
        __asm__ volatile (
            "rep movsl"
            : "+D"(fb_phys), "+S"(backbuffer), "+c"(pixels)
            : : "memory"
        );
        ret = 0;
        break;
    }

    case SYS_wm_get_backbuf: {
        uint32_t *bb = wm_get_backbuf();
        if (!bb) { ret = -1; break; }
        *(uint64_t *)a1 = (uint64_t)(uintptr_t)bb;
        ret = 0;
        break;
    }
    
    case SYS_wm_get_info: {
        *(uint32_t *)a1 = wm_get_scr_w();
        *(uint32_t *)a2 = wm_get_scr_h();
        ret = 0;
        break;
    }
    
    case SYS_wm_flush: {
        wm_flush();
        ret = 0;
        break;
    }

    default:
        console_printf("syscall %d desconhecida~ baka!\n", (unsigned)num);
        ret = -1;
        break;
    }

    regs[14] = ret;
}
