/* ♥ OWT App ~ Janela com label e botão! */
#include "../lib/kernel_stubs.h"
#include "../lib/owt/owt.h"

int main(void) {
    fb_init();
    
    if (!g_fb || g_fb_w == 0) return 1;
    
    /* Fundo cinza */
    for (int i = 0; i < g_fb_w * g_fb_h; i++) {
        g_fb[i] = 0xFF2D2D3A;
    }
    
    /* Cria uma janela OWT */
    owt_window_t *win = owt_window_create("Ovsb.OS Desktop", 100, 60, 600, 400);
    if (!win) return 1;
    
    /* Desenha a janela */
    owt_window_draw(win);
    
    /* Flush */
    syscall6(SYS_wm_flush, 0, 0, 0, 0, 0, 0);
    
    /* Espera ESC */
    char c;
    do {
        syscall6(3, 0, (long)&c, 1, 0, 0, 0);
    } while (c != 27);
    
    return 0;
}
