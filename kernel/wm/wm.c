/* ♥ WM ~ window manager! janelinhas graficas~ */
#include "wm.h"
#include "../../kernel/memory.h"

static uint32_t *backbuf;
static uint32_t *framebuf;
static int scr_w, scr_h, stride;

void wm_init(uint32_t *fb, int w, int h, int st) {
    framebuf = fb;
    scr_w = w;
    scr_h = h;
    stride = st;
    size_t size = (size_t)w * h * sizeof(uint32_t);
    backbuf = (uint32_t *)kmalloc(size);
    if (backbuf) {
        for (int i = 0; i < w * h; i++)
            backbuf[i] = 0xFF1A1A2E;
    }
}

uint32_t *wm_get_backbuf(void) { return backbuf; }
uint32_t wm_get_stride(void) { return (uint32_t)stride; }
int wm_get_scr_w(void) { return scr_w; }
int wm_get_scr_h(void) { return scr_h; }

void wm_flush(void) {
    if (!backbuf || !framebuf) return;
    for (int y = 0; y < scr_h; y++)
        for (int x = 0; x < scr_w; x++)
            framebuf[y * stride + x] = backbuf[y * (uint32_t)scr_w + x];
}