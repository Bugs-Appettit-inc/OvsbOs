/* ♥ Desktop Ring 3 ~ SEM SSE! */
static long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8  asm("r8")  = a5;
    register long r9  asm("r9")  = a6;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
        : "memory", "r11", "rcx"
    );
    return ret;
}

int main(void) {
    unsigned long fb_addr = 0;
    unsigned int w = 0, h = 0, p = 0;
    
    long ret = syscall6(202, (long)&fb_addr, (long)&w, (long)&h, (long)&p, 0, 0);
    if (ret < 0 || fb_addr == 0) {
        syscall6(1, 0, 0, 0, 0, 0, 0);
        return 1;
    }
    
    unsigned int *fb = (unsigned int *)(unsigned long)fb_addr;
    
    /* Pinta a tela de azul - loop simples sem SSE */
    unsigned int i;
    unsigned int total = w * h;
    for (i = 0; i < total; i++) {
        fb[i] = 0xFF1A1A2E;
    }
    
    /* Retangulo vermelho */
    int x, y;
    for (y = 200; y < 400; y++) {
        for (x = 300; x < 600; x++) {
            fb[y * (p/4) + x] = 0xFFFF0000;
        }
    }
    
    syscall6(203, (long)fb, 0, 0, 0, 0, 0);
    syscall6(1, 0, 0, 0, 0, 0, 0);
    return 0;
}
