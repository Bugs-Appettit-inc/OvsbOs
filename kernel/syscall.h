#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>

/* ♥ SYSCALLS ~ Todas as chamadas de sistema!
 * Dica: int 0x80 com numero em RAX, args em RDI, RSI, RDX, R10~
 * Retorno em RAX! Se der erro, retorna -1~ hihi burro!
 * Tabela completa pros programinhas ring 3~ kyun! */

#define SYS_exit         1
#define SYS_read         3
#define SYS_write        4
#define SYS_open         5
#define SYS_close        6
#define SYS_unlink       10
#define SYS_getpid       20
#define SYS_getuid       24
#define SYS_geteuid      25
#define SYS_access       33
#define SYS_getgid       47
#define SYS_getegid      48
#define SYS_ioctl        54
#define SYS_munmap       73
#define SYS_mprotect     74
#define SYS_mkdir2       136
#define SYS_rmdir2       137
#define SYS_sigaction    134
#define SYS_sigreturn    173
#define SYS_gettimeofday 116
#define SYS_mmap         197
#define SYS_kbhit        198
#define SYS_lseek        200
#define SYS_stat         188
#define SYS_fstat        189
#define SYS_lstat        199

/* ♥ Syscalls de display (framebuffer) */
#define SYS_disp_get_fb  202
#define SYS_disp_flush   203
#define SYS_wm_get_backbuf 204
#define SYS_wm_get_info    205
#define SYS_wm_flush       206

void syscall_init(void);
void syscall_handler(uint64_t *regs);

#endif
