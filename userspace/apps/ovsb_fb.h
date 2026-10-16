/* ovsb_fb.h — backend grafico/texto opcional p/ apps do OvsbOS.
 *
 * Tenta o framebuffer do kernel via syscalls TipOS raw (200/201/202/206).
 * ATENCAO: esses numeros colidem com syscalls Linux no host (200=tkill!),
 * por isso NENHUMA chamada raw e feita a menos que o ambiente seja OvsbOS
 * (variavel de ambiente OVSBOS=1, exportada pelo desktop do guest).
 * Sem isso, tudo vira no-op seguro e o app roda em modo texto puro.
 */
#ifndef OVSB_FB_H
#define OVSB_FB_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "font8x8.h"

/* Teclas logicas retornadas por app_kbd_get() (>=256 = especiais). */
#define APP_KEY_NONE  0
#define APP_KEY_UP    256
#define APP_KEY_DOWN  257
#define APP_KEY_LEFT  258
#define APP_KEY_RIGHT 259
#define APP_KEY_HOME  260
#define APP_KEY_END   261
#define APP_KEY_PGUP  262
#define APP_KEY_PGDN  263
#define APP_KEY_DEL   264
#define APP_KEY_ESC   27
#define APP_KEY_BS    8
#define APP_KEY_ENTER 13
#define APP_KEY_TAB   9

typedef struct {
    uint32_t *pix;
    unsigned long w, h, pitch_px;
    int ok;
} app_fb_t;

#ifdef __x86_64__
static __attribute__((unused)) long app_raw_sys(long n, long a, long b, long c, long d) {
    register long r10 __asm__("r10") = d;
    long r;
    __asm__ volatile("syscall"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10)
                     : "rcx", "r11", "memory");
    return r;
}
#else
static __attribute__((unused)) long app_raw_sys(long n, long a, long b, long c, long d) {
    (void)n; (void)a; (void)b; (void)c; (void)d;
    return -1;
}
#endif

/* 1 se estamos no guest OvsbOS (opt-in explicito). */
static __attribute__((unused)) int app_is_ovsbos(void) {
    const char *e = getenv("OVSBOS");
    return e && e[0] == '1';
}

static __attribute__((unused)) int app_fb_init(app_fb_t *fb) {
    memset(fb, 0, sizeof(*fb));
    if (!app_is_ovsbos()) return 0;
    {
        unsigned long long addr = 0, w = 0, h = 0, pitch = 0;
        long r = app_raw_sys(200, (long)&addr, (long)&w, (long)&h,
                             (long)&pitch);
        if (r != 0 || addr == 0 || w == 0 || h == 0) return 0;
        fb->pix = (uint32_t *)(uintptr_t)addr;
        fb->w = w;
        fb->h = h;
        fb->pitch_px = pitch / 4;
        fb->ok = 1;
        /* habilita fila de eventos de teclado */
        app_raw_sys(206, 1, 0, 0, 0);
        return 1;
    }
}

static __attribute__((unused)) void app_fb_flush(app_fb_t *fb) {
    if (fb->ok) app_raw_sys(201, 0, 0, 0, 0);
}

static __attribute__((unused)) void app_px(app_fb_t *fb, unsigned long x, unsigned long y,
                   uint32_t c) {
    if (x < fb->w && y < fb->h)
        fb->pix[y * fb->pitch_px + x] = c;
}

static __attribute__((unused)) void app_rect(app_fb_t *fb, unsigned long x, unsigned long y,
                     unsigned long w, unsigned long h, uint32_t c) {
    unsigned long j, i;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            app_px(fb, x + i, y + j, c);
}

static __attribute__((unused)) void app_char(app_fb_t *fb, unsigned long x, unsigned long y,
                     char ch, uint32_t fg, uint32_t bg) {
    unsigned char u = (unsigned char)ch;
    const unsigned char *g;
    int r, bit;
    if (u < 32 || u > 126) u = '?';
    g = app_font8x8[u - 32];
    for (r = 0; r < 8; r++)
        for (bit = 0; bit < 8; bit++)
            app_px(fb, x + (unsigned long)bit, y + (unsigned long)r,
                   (g[r] & (0x80 >> bit)) ? fg : bg);
}

static __attribute__((unused)) void app_text(app_fb_t *fb, unsigned long x, unsigned long y,
                     const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        app_char(fb, x, y, *s, fg, bg);
        x += 8;
        s++;
    }
}

/* scancode set-1 (press) -> ASCII basico; shift so p/ letras. */
static __attribute__((unused)) char app_scan_ascii(unsigned char sc, int shift) {
    static const char map[58] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
        0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
        0, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
        '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        0, 0, 0, ' '
    };
    char c;
    if (sc >= sizeof(map)) return 0;
    c = map[sc];
    if (shift && c >= 'a' && c <= 'z') c = (char)(c - 32);
    return c;
}

/* Retorna: 0-255 ASCII, APP_KEY_* especial, APP_KEY_NONE se vazio. */
static __attribute__((unused)) int app_kbd_get(void) {
    static int shift = 0;
    static int ext = 0;
    long v;
    unsigned char raw, sc;
    if (!app_is_ovsbos()) return APP_KEY_NONE;
    v = app_raw_sys(206, 0, 0, 0, 0);
    if (v < 0 || v >= 256) return APP_KEY_NONE;
    raw = (unsigned char)v;
    if (raw == 0xE0) { ext = 1; return APP_KEY_NONE; }
    sc = raw & 0x7F;
    if (raw & 0x80) { /* release */
        if (!ext && (sc == 0x2A || sc == 0x36)) shift = 0;
        ext = 0;
        return APP_KEY_NONE;
    }
    if (ext) {
        ext = 0;
        switch (sc) {
        case 0x48: return APP_KEY_UP;
        case 0x50: return APP_KEY_DOWN;
        case 0x4B: return APP_KEY_LEFT;
        case 0x4D: return APP_KEY_RIGHT;
        case 0x47: return APP_KEY_HOME;
        case 0x4F: return APP_KEY_END;
        case 0x49: return APP_KEY_PGUP;
        case 0x51: return APP_KEY_PGDN;
        case 0x53: return APP_KEY_DEL;
        default: return APP_KEY_NONE;
        }
    }
    switch (sc) {
    case 0x2A: case 0x36: shift = 1; return APP_KEY_NONE;
    case 0x0E: return APP_KEY_BS;
    case 0x1C: return APP_KEY_ENTER;
    case 0x01: return APP_KEY_ESC;
    case 0x0F: return APP_KEY_TAB;
    case 0x47: return APP_KEY_HOME;
    case 0x4F: return APP_KEY_END;
    case 0x49: return APP_KEY_PGUP;
    case 0x51: return APP_KEY_PGDN;
    case 0x53: return APP_KEY_DEL;
    case 0x48: return APP_KEY_UP;
    case 0x50: return APP_KEY_DOWN;
    case 0x4B: return APP_KEY_LEFT;
    case 0x4D: return APP_KEY_RIGHT;
    default: break;
    }
    {
        char c = app_scan_ascii(sc, shift);
        return c ? (unsigned char)c : APP_KEY_NONE;
    }
}

/* mouse: dx, dy relativos + bitmask de botoes. 0 = indisponivel. */
static __attribute__((unused)) int app_mouse_get(long *dx, long *dy, long *btn) {
    long r;
    if (!app_is_ovsbos()) return 0;
    r = app_raw_sys(202, (long)dx, (long)dy, (long)btn, 0);
    return r == 0;
}

#endif
