/* image.c — BMP + PPM (standalone). PNG em png.c (opcional). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

img_t *img_new(int w, int h) {
    img_t *im;
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return NULL;
    im = malloc(sizeof(*im));
    if (!im) return NULL;
    im->px = calloc((size_t)w * (size_t)h, sizeof(unsigned int));
    if (!im->px) { free(im); return NULL; }
    im->w = w;
    im->h = h;
    return im;
}

void img_free(img_t *im) {
    if (!im) return;
    free(im->px);
    free(im);
}

static unsigned rd16(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned rd32(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8) |
           ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

static void wr16(FILE *f, unsigned v) {
    fputc((int)(v & 0xFF), f);
    fputc((int)((v >> 8) & 0xFF), f);
}

static void wr32(FILE *f, unsigned long v) {
    fputc((int)(v & 0xFF), f);
    fputc((int)((v >> 8) & 0xFF), f);
    fputc((int)((v >> 16) & 0xFF), f);
    fputc((int)((v >> 24) & 0xFF), f);
}

/* ---------- BMP ---------- */
img_t *img_bmp_load(void *fp0) {
    FILE *f = fp0;
    unsigned char hd[54];
    unsigned off, w, h, bpp, comp, stride, y, x;
    int topdown = 0;
    img_t *im;
    if (fread(hd, 1, 54, f) != 54) return NULL;
    if (hd[0] != 'B' || hd[1] != 'M') return NULL;
    if (rd32(hd + 14) != 40) return NULL; /* so BITMAPINFOHEADER */
    off = rd32(hd + 10);
    w = rd32(hd + 18);
    h = rd32(hd + 22);
    bpp = rd16(hd + 28);
    comp = rd32(hd + 30);
    if (comp != 0) return NULL; /* sem compressao */
    if ((bpp != 24 && bpp != 32) || w == 0 || w > 8192) return NULL;
    if ((h & 0x80000000U) != 0) { topdown = 1; h = (~h + 1) & 0x7FFFFFFF; }
    if (h == 0 || h > 8192) return NULL;
    im = img_new((int)w, (int)h);
    if (!im) return NULL;
    stride = (w * bpp / 8 + 3) & ~3U;
    {
        unsigned char *row = malloc(stride ? stride : 1);
        if (!row) { img_free(im); return NULL; }
        if (off > 54 && fseek(f, (long)off, SEEK_SET) != 0) {
            free(row); img_free(im); return NULL;
        }
        for (y = 0; y < h; y++) {
            unsigned dst = topdown ? y : (h - 1 - y);
            if (fread(row, 1, stride, f) != stride) {
                free(row); img_free(im); return NULL;
            }
            for (x = 0; x < w; x++) {
                unsigned b = row[x * bpp / 8], g = row[x * bpp / 8 + 1];
                unsigned r = row[x * bpp / 8 + 2];
                im->px[dst * w + x] = (r << 16) | (g << 8) | b;
            }
        }
        free(row);
    }
    return im;
}

int img_bmp_save(const img_t *im, void *fp0) {
    FILE *f = fp0;
    unsigned stride, y, x, pad;
    if (!im || !f) return -1;
    stride = ((unsigned)im->w * 3 + 3) & ~3U;
    pad = stride - (unsigned)im->w * 3;
    fputc('B', f); fputc('M', f);
    wr32(f, 54 + stride * (unsigned)im->h);
    wr32(f, 0);
    wr32(f, 54);
    wr32(f, 40);
    wr32(f, (unsigned)im->w);
    wr32(f, (unsigned)im->h);
    wr16(f, 1);
    wr16(f, 24);
    wr32(f, 0);
    wr32(f, stride * (unsigned)im->h);
    wr32(f, 2835); wr32(f, 2835);
    wr32(f, 0); wr32(f, 0);
    for (y = (unsigned)im->h; y-- > 0;) {
        for (x = 0; x < (unsigned)im->w; x++) {
            unsigned c = im->px[y * (unsigned)im->w + x];
            fputc((int)(c & 0xFF), f);
            fputc((int)((c >> 8) & 0xFF), f);
            fputc((int)((c >> 16) & 0xFF), f);
        }
        for (x = 0; x < pad; x++) fputc(0, f);
    }
    return ferror(f) ? -1 : 0;
}

/* ---------- PPM P6 ---------- */
static int ppm_tok(FILE *f, char *out, int n) {
    int c, i = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (i > 0) break;
            continue;
        }
        if (i + 1 < n) out[i++] = (char)c;
    }
    out[i] = 0;
    return 0;
}

img_t *img_ppm_load(void *fp0) {
    FILE *f = fp0;
    char tok[32];
    int w, h, m, x, y;
    img_t *im;
    if (ppm_tok(f, tok, sizeof(tok)) != 0 || strcmp(tok, "P6") != 0)
        return NULL;
    if (ppm_tok(f, tok, sizeof(tok)) != 0) return NULL;
    w = atoi(tok);
    if (ppm_tok(f, tok, sizeof(tok)) != 0) return NULL;
    h = atoi(tok);
    if (ppm_tok(f, tok, sizeof(tok)) != 0) return NULL;
    m = atoi(tok);
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192 || m != 255) return NULL;
    /* NOTA: ppm_tok ja consumiu o whitespace apos maxval; os pixels
     * comecam imediatamente. Nao consumir byte extra aqui. */
    im = img_new(w, h);
    if (!im) return NULL;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int r = fgetc(f), g = fgetc(f), b = fgetc(f);
            if (b == EOF) { img_free(im); return NULL; }
            im->px[y * w + x] =
                ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b;
        }
    return im;
}

int img_ppm_save(const img_t *im, void *fp0) {
    FILE *f = fp0;
    int x, y;
    if (!im || !f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", im->w, im->h);
    for (y = 0; y < im->h; y++)
        for (x = 0; x < im->w; x++) {
            unsigned c = im->px[y * im->w + x];
            fputc((int)(c >> 16), f);
            fputc((int)((c >> 8) & 0xFF), f);
            fputc((int)(c & 0xFF), f);
        }
    return ferror(f) ? -1 : 0;
}

/* ---------- API unificada ---------- */
static const char *ext_of(const char *p) {
    const char *d = strrchr(p, '.');
    const char *s = strrchr(p, '/');
    if (!d || (s && d < s)) return "";
    return d;
}

img_t *img_load(const char *path) {
    FILE *f = fopen(path, "rb");
    const char *e;
    img_t *im = NULL;
    if (!f) return NULL;
    e = ext_of(path);
    if (strcmp(e, ".bmp") == 0 || strcmp(e, ".BMP") == 0)
        im = img_bmp_load(f);
    else if (strcmp(e, ".ppm") == 0 || strcmp(e, ".PPM") == 0)
        im = img_ppm_load(f);
#ifdef IMG_WITH_PNG
    else if (strcmp(e, ".png") == 0 || strcmp(e, ".PNG") == 0)
        im = img_png_load(f);
#endif
    else {
        /* tenta PPM, depois BMP (conteudo manda) */
        long pos = ftell(f);
        im = img_ppm_load(f);
        if (!im && pos >= 0) {
            fseek(f, pos, SEEK_SET);
            im = img_bmp_load(f);
        }
    }
    fclose(f);
    return im;
}

int img_save(const img_t *im, const char *path) {
    FILE *f = fopen(path, "wb");
    const char *e;
    int r = -1;
    if (!f) return -1;
    e = ext_of(path);
    if (strcmp(e, ".bmp") == 0 || strcmp(e, ".BMP") == 0)
        r = img_bmp_save(im, f);
    else
        r = img_ppm_save(im, f); /* default: PPM */
    fclose(f);
    return r;
}

/* ---------- ops ---------- */
img_t *img_scale(const img_t *im, int nw, int nh) {
    img_t *o;
    int x, y;
    if (!im || nw <= 0 || nh <= 0 || nw > 8192 || nh > 8192) return NULL;
    o = img_new(nw, nh);
    if (!o) return NULL;
    for (y = 0; y < nh; y++)
        for (x = 0; x < nw; x++) {
            int sx = x * im->w / nw, sy = y * im->h / nh;
            o->px[y * nw + x] = im->px[sy * im->w + sx];
        }
    return o;
}

img_t *img_crop(const img_t *im, int x, int y, int w, int h) {
    img_t *o;
    int j, i;
    if (!im || w <= 0 || h <= 0) return NULL;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > im->w) w = im->w - x;
    if (y + h > im->h) h = im->h - y;
    if (w <= 0 || h <= 0) return NULL;
    o = img_new(w, h);
    if (!o) return NULL;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            o->px[j * w + i] = im->px[(y + j) * im->w + (x + i)];
    return o;
}

void img_blend(img_t *dst, const img_t *src, int dx, int dy, int alpha) {
    int j, i;
    unsigned a;
    if (!dst || !src) return;
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    a = (unsigned)alpha;
    for (j = 0; j < src->h; j++)
        for (i = 0; i < src->w; i++) {
            int x = dx + i, y = dy + j;
            unsigned s, d, r, g, b;
            if (x < 0 || y < 0 || x >= dst->w || y >= dst->h) continue;
            s = src->px[j * src->w + i];
            d = dst->px[y * dst->w + x];
            r = ((s >> 16) & 0xFF) * a + ((d >> 16) & 0xFF) * (255 - a);
            g = ((s >> 8) & 0xFF) * a + ((d >> 8) & 0xFF) * (255 - a);
            b = (s & 0xFF) * a + (d & 0xFF) * (255 - a);
            dst->px[y * dst->w + x] =
                (((r / 255) & 0xFF) << 16) | (((g / 255) & 0xFF) << 8) |
                ((b / 255) & 0xFF);
        }
}

void img_ascii(const img_t *im, int maxw) {
    static const char *ramp = " .:-=+*#%@";
    int w, h, x, y;
    if (!im || maxw < 8) return;
    if (maxw > 160) maxw = 160;
    w = im->w > maxw ? maxw : im->w;
    h = im->h * w / im->w / 2;
    if (h < 1) h = 1;
    if (h > 60) h = 60;
    printf("[%dx%d]\n", im->w, im->h);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            unsigned c =
                im->px[(y * im->h / h) * im->w + (x * im->w / w)];
            unsigned lum =
                (((c >> 16) & 0xFF) * 30 + ((c >> 8) & 0xFF) * 59 +
                 (c & 0xFF) * 11) / 100;
            putchar(ramp[lum * 9 / 256]);
        }
        putchar('\n');
    }
}
