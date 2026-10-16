/* paint — desenho em framebuffer (--gui) ou PPM em lote (CLI).
 *
 * CLI: paint --cli OP...  (sem display: gera/altera out.ppm)
 *   new W H C        cria canvas WxH cor C (0xRRGGBB ou indice 0-15)
 *   line x0 y0 x1 y1 C | rect ... | ellas ... | fill x y C | clear C
 *   undo | save FILE | load FILE
 * GUI: paint --gui  (requer OvsbOS; mouse desenha, teclado troca ferramenta)
 *   b=pincel e=borracha l=linha r=retangulo o=elipse g=balde
 *   [/] cor, z=desfaz, s=salva (out.ppm), q=sai
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ovsb_fb.h"

#define MAXW 640
#define MAXH 480
#define UNDO_N 10

static uint32_t g_cv[MAXW * MAXH];
static int g_w = 320, g_h = 200;
static uint32_t g_undo[UNDO_N][MAXW * MAXH];
static int g_uw = 0, g_uh = 0, g_utop = 0, g_un = 0;

static const uint32_t g_pal[16] = {
    0x000000, 0xC00000, 0x00C000, 0xC0C000, 0x0000C0, 0xC000C0,
    0x00C0C0, 0xC0C0C0, 0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
    0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
};

static uint32_t parse_color(const char *s) {
    unsigned long v;
    char *e = NULL;
    if (!s || !*s) return 0xFFFFFF;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        v = strtoul(s, &e, 16);
        if (e && !*e) return (uint32_t)(v & 0xFFFFFF);
    }
    v = strtoul(s, &e, 10);
    if (e && !*e && v < 16) return g_pal[v];
    return 0xFFFFFF;
}

static void pset(int x, int y, uint32_t c) {
    if (x >= 0 && y >= 0 && x < g_w && y < g_h)
        g_cv[y * MAXW + x] = c & 0xFFFFFF;
}

static uint32_t pget(int x, int y) {
    if (x >= 0 && y >= 0 && x < g_w && y < g_h)
        return g_cv[y * MAXW + x];
    return 0xFFFFFFFF;
}

static void snapshot(void) {
    int i = g_utop % UNDO_N;
    memcpy(g_undo[i], g_cv, sizeof(g_cv));
    g_uw = g_w;
    g_uh = g_h;
    g_utop++;
    if (g_un < UNDO_N) g_un++;
}

static int undo(void) {
    int i;
    if (g_un == 0) return -1;
    g_utop--;
    g_un--;
    i = (g_utop % UNDO_N + UNDO_N) % UNDO_N;
    g_w = g_uw;
    g_h = g_uh;
    memcpy(g_cv, g_undo[i], sizeof(g_cv));
    return 0;
}

static void pline(int x0, int y0, int x1, int y1, uint32_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        pset(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void prect(int x0, int y0, int x1, int y1, uint32_t c, int fill) {
    int x, y, xa = x0 < x1 ? x0 : x1, xb = x0 < x1 ? x1 : x0;
    int ya = y0 < y1 ? y0 : y1, yb = y0 < y1 ? y1 : y0;
    if (fill) {
        for (y = ya; y <= yb; y++)
            for (x = xa; x <= xb; x++)
                pset(x, y, c);
    } else {
        for (x = xa; x <= xb; x++) { pset(x, ya, c); pset(x, yb, c); }
        for (y = ya; y <= yb; y++) { pset(xa, y, c); pset(xb, y, c); }
    }
}

static void pell(int x0, int y0, int x1, int y1, uint32_t c, int fill) {
    int xa = x0 < x1 ? x0 : x1, xb = x0 < x1 ? x1 : x0;
    int ya = y0 < y1 ? y0 : y1, yb = y0 < y1 ? y1 : y0;
    double rx = (xb - xa) / 2.0, ry = (yb - ya) / 2.0;
    double cx = xa + rx, cy = ya + ry;
    int x, y;
    if (rx < 0.5 || ry < 0.5) { prect(xa, ya, xb, yb, c, fill); return; }
    for (y = ya; y <= yb; y++) {
        for (x = xa; x <= xb; x++) {
            double v = ((x - cx) * (x - cx)) / (rx * rx) +
                       ((y - cy) * (y - cy)) / (ry * ry);
            if (fill ? v <= 1.0 : (v <= 1.0 && v >= 1.0 - 2.0 / (rx + ry)))
                pset(x, y, c);
        }
    }
}

static void pfill(int x, int y, uint32_t c) {
    /* flood fill com pilha explicita (sem recursao: imagem binaria ok). */
    static int sx[MAXW * MAXH / 4];
    static int sy[MAXW * MAXH / 4];
    int n = 0;
    uint32_t tgt = pget(x, y);
    c &= 0xFFFFFF;
    if (tgt == 0xFFFFFFFF || tgt == c) return;
    sx[n] = x;
    sy[n] = y;
    n++;
    while (n > 0) {
        int cx, cy;
        n--;
        cx = sx[n];
        cy = sy[n];
        if (pget(cx, cy) != tgt) continue;
        pset(cx, cy, c);
        if (cx > 0 && n < (int)(sizeof(sx) / sizeof(sx[0]))) {
            sx[n] = cx - 1; sy[n] = cy; n++;
        }
        if (cx + 1 < g_w && n < (int)(sizeof(sx) / sizeof(sx[0]))) {
            sx[n] = cx + 1; sy[n] = cy; n++;
        }
        if (cy > 0 && n < (int)(sizeof(sx) / sizeof(sx[0]))) {
            sx[n] = cx; sy[n] = cy - 1; n++;
        }
        if (cy + 1 < g_h && n < (int)(sizeof(sx) / sizeof(sx[0]))) {
            sx[n] = cx; sy[n] = cy + 1; n++;
        }
    }
}

static int ppm_save(const char *path) {
    FILE *f = fopen(path, "wb");
    int y, x;
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
    for (y = 0; y < g_h; y++)
        for (x = 0; x < g_w; x++) {
            uint32_t c = g_cv[y * MAXW + x];
            unsigned char b[3];
            b[0] = (unsigned char)(c >> 16);
            b[1] = (unsigned char)(c >> 8);
            b[2] = (unsigned char)c;
            if (fwrite(b, 1, 3, f) != 3) { fclose(f); return -1; }
        }
    fclose(f);
    return 0;
}

static int ppm_load(const char *path) {
    FILE *f = fopen(path, "rb");
    int w = 0, h = 0, m = 0, y, x;
    if (!f) return -1;
    snapshot();
    if (fscanf(f, "P6 %d %d %d", &w, &h, &m) != 3) {
        fclose(f);
        undo();
        return -1;
    }
    if (w <= 0 || h <= 0 || w > MAXW || h > MAXH || m != 255) {
        fclose(f);
        undo();
        return -1;
    }
    if (fgetc(f) == EOF) { fclose(f); undo(); return -1; }
    g_w = w;
    g_h = h;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            unsigned char b[3];
            if (fread(b, 1, 3, f) != 3) { fclose(f); undo(); return -1; }
            g_cv[y * MAXW + x] =
                ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
        }
    fclose(f);
    return 0;
}

static void cli_help(void) {
    printf("uso: paint --cli OP... | paint --gui\n"
           "  new W H C | line x0 y0 x1 y1 C | rect x0 y0 x1 y1 C [f]\n"
           "  ell x0 y0 x1 y1 C [f] | fill x y C | clear C | undo\n"
           "  save F | load F   (PPM P6; C=0xRRGGBB ou 0-15)\n");
}

/* GUI via framebuffer: pledge interativo minimo. */
static void gui_run(void) {
    app_fb_t fb;
    int quit = 0, tool = 0, ci = 15, x0 = -1, y0 = -1;
    long mx = 0, my = 0, dx = 0, dy = 0, btn = 0, pbtn = 0;
    uint32_t fg;
    int k, i;
    if (!app_fb_init(&fb)) {
        printf("sem framebuffer neste ambiente (rode no OvsbOS com OVSBOS=1)\n");
        return;
    }
    g_w = fb.w > MAXW ? MAXW : (int)fb.w;
    g_h = fb.h > MAXH ? MAXH : (int)fb.h;
    mx = g_w / 2;
    my = g_h / 2;
    while (!quit) {
        k = app_kbd_get();
        switch (k) {
        case 'b': tool = 0; break;
        case 'e': tool = 1; break;
        case 'l': tool = 2; break;
        case 'r': tool = 3; break;
        case 'o': tool = 4; break;
        case 'g': tool = 5; break;
        case '[': ci = (ci + 15) % 16; break;
        case ']': ci = (ci + 1) % 16; break;
        case 'z': undo(); break;
        case 's': ppm_save("out.ppm"); break;
        case 'q': case APP_KEY_ESC: quit = 1; break;
        default: break;
        }
        fg = tool == 1 ? 0x000000 : g_pal[ci];
        if (app_mouse_get(&dx, &dy, &btn)) {
            mx += dx;
            my += dy;
            if (mx < 0) mx = 0;
            if (my < 0) my = 0;
            if (mx >= g_w) mx = g_w - 1;
            if (my >= g_h) my = g_h - 1;
        }
        if (btn && !pbtn) { /* press: inicia */
            snapshot();
            x0 = (int)mx;
            y0 = (int)my;
            if (tool == 0) pset(x0, y0, fg);
            else if (tool == 5) pfill(x0, y0, fg);
        } else if (btn && pbtn && x0 >= 0) {
            if (tool == 0) pline(x0, y0, (int)mx, (int)my, fg);
            else if (tool == 1) {
                x0 = (int)mx; y0 = (int)my;
            }
            if (tool == 0 || tool == 1) { x0 = (int)mx; y0 = (int)my; }
        } else if (!btn && pbtn && x0 >= 0) {
            if (tool == 2) { undo(); snapshot(); pline(x0, y0, (int)mx, (int)my, fg); }
            else if (tool == 3) { undo(); snapshot(); prect(x0, y0, (int)mx, (int)my, fg, 0); }
            else if (tool == 4) { undo(); snapshot(); pell(x0, y0, (int)mx, (int)my, fg, 0); }
            x0 = -1;
        }
        pbtn = btn;
        for (i = 0; i < g_w * g_h; i++) {
            int x = i % g_w, y = i / g_w;
            if ((unsigned long)x < fb.w && (unsigned long)y < fb.h)
                fb.pix[y * fb.pitch_px + x] = g_cv[y * MAXW + x] | 0xFF000000;
        }
        app_rect(&fb, (unsigned long)mx - 2, (unsigned long)my, 5, 1, 0xFFFFFFFF);
        app_rect(&fb, (unsigned long)mx, (unsigned long)my - 2, 1, 5, 0xFFFFFFFF);
        app_fb_flush(&fb);
    }
}

int main(int argc, char **argv) {
    int i = 1;
    if (argc >= 2 && strcmp(argv[1], "--gui") == 0) {
        gui_run();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "--cli") == 0) i = 2;
    if (i >= argc) { cli_help(); return 1; }
    memset(g_cv, 0, sizeof(g_cv));
    while (i < argc) {
        const char *op = argv[i++];
        if (strcmp(op, "new") == 0 && i + 2 < argc) {
            g_w = atoi(argv[i]);
            g_h = atoi(argv[i + 1]);
            if (g_w < 1) g_w = 1;
            if (g_h < 1) g_h = 1;
            if (g_w > MAXW) g_w = MAXW;
            if (g_h > MAXH) g_h = MAXH;
            {
                uint32_t c = parse_color(argv[i + 2]);
                int x, y;
                for (y = 0; y < g_h; y++)
                    for (x = 0; x < g_w; x++)
                        g_cv[y * MAXW + x] = c;
            }
            snapshot();
            i += 3;
        } else if (strcmp(op, "clear") == 0 && i < argc) {
            uint32_t c = parse_color(argv[i++]);
            int x, y;
            snapshot();
            for (y = 0; y < g_h; y++)
                for (x = 0; x < g_w; x++)
                    g_cv[y * MAXW + x] = c;
        } else if (strcmp(op, "line") == 0 && i + 4 < argc) {
            snapshot();
            pline(atoi(argv[i]), atoi(argv[i + 1]), atoi(argv[i + 2]),
                  atoi(argv[i + 3]), parse_color(argv[i + 4]));
            i += 5;
        } else if ((strcmp(op, "rect") == 0 || strcmp(op, "ell") == 0) &&
                   i + 4 < argc) {
            int f = (i + 5 < argc && argv[i + 5][0] == 'f' &&
                       argv[i + 5][1] == 0) ? 1 : 0;
            snapshot();
            if (op[0] == 'r')
                prect(atoi(argv[i]), atoi(argv[i + 1]), atoi(argv[i + 2]),
                      atoi(argv[i + 3]), parse_color(argv[i + 4]), f);
            else
                pell(atoi(argv[i]), atoi(argv[i + 1]), atoi(argv[i + 2]),
                     atoi(argv[i + 3]), parse_color(argv[i + 4]), f);
            i += f ? 6 : 5;
        } else if (strcmp(op, "fill") == 0 && i + 2 < argc) {
            snapshot();
            pfill(atoi(argv[i]), atoi(argv[i + 1]), parse_color(argv[i + 2]));
            i += 3;
        } else if (strcmp(op, "undo") == 0) {
            if (undo() != 0) { printf("nada a desfazer\n"); return 1; }
        } else if (strcmp(op, "save") == 0 && i < argc) {
            if (ppm_save(argv[i++]) != 0) {
                printf("erro ao salvar\n");
                return 1;
            }
        } else if (strcmp(op, "load") == 0 && i < argc) {
            if (ppm_load(argv[i++]) != 0) {
                printf("erro ao carregar (PPM P6?)\n");
                return 1;
            }
        } else {
            cli_help();
            return 1;
        }
    }
    printf("paint: %dx%d ok\n", g_w, g_h);
    return 0;
}
