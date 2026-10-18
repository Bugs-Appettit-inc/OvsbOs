/* syswrite_test — teste de sys_write direto */
#include <stdint.h>

static inline void sys_write(int fd, const char *buf, int len) {
    long ret;
    uint64_t fd64 = (uint64_t)fd;
    uint64_t buf64 = (uint64_t)(uintptr_t)buf;
    uint64_t len64 = (uint64_t)len;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(4), "D"(fd64), "S"(buf64), "d"(len64)
        : "rcx", "r11", "memory"
    );
    (void)ret;
}

int main(void) {
    const char *msg = "SYSWRITE_TEST: hello from userspace\n";
    sys_write(1, msg, 29);
    return 0;
}
