#include "../lib/kernel_stubs.h"

int main(void) {
    fb_init();
    
    if (!g_fb || g_fb_w == 0) return 1;
    
    /* Tudo vermelho */
    for (int i = 0; i < g_fb_w * g_fb_h; i++) {
        g_fb[i] = 0xFFFF0000;
    }
    
    /* Barra azul */
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < g_fb_w; x++)
            g_fb[y * g_fb_w + x] = 0xFF0000FF;
    
    /* Quadrados verdes */
    for (int y = 100; y < 130; y++)
        for (int x = 100; x < 200; x++)
            g_fb[y * g_fb_w + x] = 0xFF00FF00;
    
    for (int y = 300; y < 330; y++)
        for (int x = 500; x < 600; x++)
            g_fb[y * g_fb_w + x] = 0xFF00FF00;
    
    /* Quadrado BRANCO */
    for (int y = 400; y < 430; y++)
        for (int x = 400; x < 500; x++)
            g_fb[y * g_fb_w + x] = 0xFFFFFFFF;
    
    syscall6(SYS_wm_flush, 0, 0, 0, 0, 0, 0);
    
    /* LOOP INFINITO - espera tecla ESC */
    char c;
    do {
        syscall6(3, 0, (long)&c, 1, 0, 0, 0);  /* SYS_read stdin */
    } while (c != 27);  /* ESC */
    
    return 0;
}
