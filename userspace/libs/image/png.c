/* png.c — decoder PNG minimo (8-bit: gray/RGB/RGBA; filtros 0-4).
 * inflate proprio (stored/fixed/dynamic). Sem dependencias. SEM SIMD.
 * Suporta: IHDR, PLTE(tRNS basico p/ indexado? NAO — indexado rejeitado),
 * IDAT concatenado, IEND. Interlace Adam7: NAO (rejeita).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

img_t *img_png_load(void *fp0);

typedef struct {
    const unsigned char *p, *end;
    unsigned bitbuf;
    int bitn;
} bits_t;

static int bbit(bits_t *b) {
    if (b->bitn == 0) {
        if (b->p >= b->end) return -1;
        b->bitbuf = *b->p++;
        b->bitn = 8;
    }
    {
        int v = (int)(b->bitbuf & 1);
        b->bitbuf >>= 1;
        b->bitn--;
        return v;
    }
}

static int bbits(bits_t *b, int n) {
    int v = 0, i;
    for (i = 0; i < n; i++) {
        int bit = bbit(b);
        if (bit < 0) return -1;
        v |= bit << i;
    }
    return v;
}

#define HMAX 15
typedef struct {
    short cnt[HMAX + 1];
    short sym[288];
} huff_t;

static int huff_build(huff_t *h, const int *lens, int n) {
    int offs[HMAX + 2], i, len;
    memset(h, 0, sizeof(*h));
    for (i = 0; i < n; i++)
        if (lens[i] > HMAX) return -1;
        else h->cnt[lens[i]]++;
    h->cnt[0] = 0;
    offs[1] = 0;
    for (len = 1; len <= HMAX; len++)
        offs[len + 1] = (short)(offs[len] + h->cnt[len]);
    for (i = 0; i < n; i++)
        if (lens[i] != 0) h->sym[offs[lens[i]]++] = (short)i;
    return 0;
}

static int huff_dec(bits_t *b, huff_t *h) {
    int code = 0, first = 0, idx = 0, len;
    for (len = 1; len <= HMAX; len++) {
        int bit = bbit(b);
        if (bit < 0) return -1;
        code |= bit;
        {
            int cnt = h->cnt[len];
            if (code - first < cnt)
                return h->sym[idx + (code - first)];
            idx += cnt;
            first = (first + cnt) << 1;
            code <<= 1;
        }
    }
    return -1;
}

static int inflate_block(bits_t *b, huff_t *lit, huff_t *dst,
                         unsigned char *out, unsigned *opos,
                         unsigned oend) {
    for (;;) {
        int s = huff_dec(b, lit);
        if (s < 0) return -1;
        if (s < 256) {
            if (*opos >= oend) return -1;
            out[(*opos)++] = (unsigned char)s;
        } else if (s == 256) {
            return 0;
        } else {
            static const short lbase[29] = {
                3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23,
                27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131,
                163, 195, 227, 258
            };
            static const char lext[29] = {
                0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
            };
            static const short dbase[30] = {
                1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97,
                129, 193, 257, 385, 513, 769, 1025, 1537, 2049,
                3073, 4097, 6145, 8193, 12289, 16385, 24577
            };
            static const char dext[30] = {
                0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
            };
            int len, dist, e, i;
            if (s > 285) return -1;
            len = lbase[s - 257];
            e = bbits(b, lext[s - 257]);
            if (e < 0) return -1;
            len += e;
            s = huff_dec(b, dst);
            if (s < 0 || s > 29) return -1;
            dist = dbase[s];
            e = bbits(b, dext[s]);
            if (e < 0) return -1;
            dist += e;
            if ((unsigned)dist > *opos) return -1;
            for (i = 0; i < len; i++) {
                if (*opos >= oend) return -1;
                out[*opos] = out[*opos - (unsigned)dist];
                (*opos)++;
            }
        }
    }
}

static int inflate(const unsigned char *in, unsigned ilen,
                   unsigned char *out, unsigned olen) {
    bits_t b;
    unsigned opos = 0;
    int final = 0;
    b.p = in;
    b.end = in + ilen;
    b.bitbuf = 0;
    b.bitn = 0;
    while (!final) {
        int type;
        final = bbit(&b);
        type = bbits(&b, 2);
        if (final < 0 || type < 0) return -1;
        if (type == 0) {
            unsigned len, nlen, i;
            b.bitn = 0; /* alinha */
            if (b.end - b.p < 4) return -1;
            len = (unsigned)b.p[0] | ((unsigned)b.p[1] << 8);
            nlen = (unsigned)b.p[2] | ((unsigned)b.p[3] << 8);
            b.p += 4;
            if ((len ^ nlen) != 0xFFFF) return -1;
            if ((unsigned)(b.end - b.p) < len) return -1;
            if (opos + len > olen) return -1;
            for (i = 0; i < len; i++) out[opos++] = *b.p++;
        } else if (type == 1 || type == 2) {
            huff_t lit, dst;
            int lens[288 + 32], nlit, ndst, nhcl, i;
            if (type == 1) {
                for (i = 0; i < 144; i++) lens[i] = 8;
                for (; i < 256; i++) lens[i] = 9;
                for (; i < 280; i++) lens[i] = 7;
                for (; i < 288; i++) lens[i] = 8;
                if (huff_build(&lit, lens, 288) != 0) return -1;
                for (i = 0; i < 32; i++) lens[i] = 5;
                if (huff_build(&dst, lens, 32) != 0) return -1;
            } else {
                static const char order[19] = {
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                    11, 4, 12, 3, 13, 2, 14, 1, 15
                };
                int cl[19], l = 0;
                nlit = bbits(&b, 5) + 257;
                ndst = bbits(&b, 5) + 1;
                nhcl = bbits(&b, 4) + 4;
                if (nlit > 288 || ndst > 32) return -1;
                for (i = 0; i < 19; i++) cl[i] = 0;
                for (i = 0; i < nhcl; i++) {
                    int v = bbits(&b, 3);
                    if (v < 0) return -1;
                    cl[(int)order[i]] = v;
                }
                {
                    huff_t hc;
                    if (huff_build(&hc, cl, 19) != 0) return -1;
                    while (l < nlit + ndst) {
                        int s = huff_dec(&b, &hc);
                        if (s < 0) return -1;
                        if (s < 16) {
                            lens[l++] = s;
                        } else {
                            int rep = 0, v = 0, k;
                            if (s == 16) {
                                int e = bbits(&b, 2);
                                if (e < 0 || l == 0) return -1;
                                rep = 3 + e;
                                v = lens[l - 1];
                            } else if (s == 17) {
                                int e = bbits(&b, 3);
                                if (e < 0) return -1;
                                rep = 3 + e;
                                v = 0;
                            } else {
                                int e = bbits(&b, 7);
                                if (e < 0) return -1;
                                rep = 11 + e;
                                v = 0;
                            }
                            if (l + rep > nlit + ndst) return -1;
                            for (k = 0; k < rep; k++) lens[l++] = v;
                        }
                    }
                }
                if (huff_build(&lit, lens, nlit) != 0) return -1;
                if (huff_build(&dst, lens + nlit, ndst) != 0) return -1;
            }
            if (inflate_block(&b, &lit, &dst, out, &opos, olen) != 0)
                return -1;
            /* 256 termina o BLOCO, nao o stream: volta ao loop para ler o
             * proximo header (final/type). Se final==1, o while sai. */
        } else {
            return -1;
        }
    }
    return (opos == olen) ? 0 : -1;
}

/* ---------- PNG ---------- */
static unsigned rd32be(const unsigned char *p) {
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) |
           ((unsigned)p[2] << 8) | p[3];
}

img_t *img_png_load(void *fp0) {
    FILE *f = fp0;
    static const unsigned char magic[8] = {
        137, 80, 78, 71, 13, 10, 26, 10
    };
    unsigned char hd[8], *idat = NULL, *raw = NULL;
    unsigned idlen = 0, idcap = 0;
    unsigned w = 0, h = 0, bd = 0, ct = 0;
    int have_ihdr = 0, done = 0;
    img_t *im = NULL;
    if (fread(hd, 1, 8, f) != 8 || memcmp(hd, magic, 8) != 0) return NULL;
    while (!done) {
        unsigned char ch[8], *data;
        unsigned len, type;
        if (fread(ch, 1, 8, f) != 8) break;
        len = rd32be(ch);
        type = rd32be(ch + 4);
        if (len > (1U << 28)) break;
        data = malloc(len ? len : 1);
        if (!data) break;
        if (len && fread(data, 1, len, f) != len) {
            free(data);
            break;
        }
        if (fseek(f, 4, SEEK_CUR) != 0) { free(data); break; } /* CRC */
        if (type == 0x49484452) { /* IHDR */
            if (len != 13 || have_ihdr) { free(data); break; }
            w = rd32be(data);
            h = rd32be(data + 4);
            bd = data[8];
            ct = data[9];
            if (data[10] != 0 || data[11] != 0 || data[12] != 0) {
                free(data);
                break;
            }
            if (w == 0 || h == 0 || w > 8192 || h > 8192 || bd != 8) {
                free(data);
                break;
            }
            if (ct != 0 && ct != 2 && ct != 6) {
                free(data);
                break; /* so gray/RGB/RGBA */
            }
            have_ihdr = 1;
        } else if (type == 0x49444154) { /* IDAT */
            if (len && idlen + len < idlen) { free(data); break; }
            if (idlen + len + 1 > idcap) {
                unsigned nc = idlen + len + 65536;
                unsigned char *ni = realloc(idat, nc);
                if (!ni) { free(data); break; }
                idat = ni;
                idcap = nc;
            }
            if (len) memcpy(idat + idlen, data, len);
            idlen += len;
        } else if (type == 0x49454E44) { /* IEND */
            done = 1;
        }
        free(data);
    }
    if (!have_ihdr || !done || idlen < 2) goto out;
    {
        unsigned chn = (ct == 0) ? 1 : (ct == 2) ? 3 : 4;
        unsigned stride = w * chn;
        unsigned need = (stride + 1) * h;
        unsigned x, y;
        unsigned char *prev, *cur;
        if (idat[0] != 0x78) goto out; /* zlib: so CMF 0x78xx */
        if (idlen < 6) goto out; /* header(2) + adler(4) minimos */
        raw = malloc(need);
        prev = malloc(stride + 1);
        cur = malloc(stride + 1);
        if (!raw || !prev || !cur) goto out;
        if (inflate(idat + 2, idlen - 6, raw, need) != 0)
            goto out;
        im = img_new((int)w, (int)h);
        if (!im) goto out;
        memset(prev, 0, stride);
        for (y = 0; y < h; y++) {
            unsigned char *row = raw + y * (stride + 1);
            int flt = row[0];
            unsigned i, a, b, c;
            memcpy(cur, row + 1, stride);
            for (i = 0; i < stride; i++) {
                a = (i >= chn) ? cur[i - chn] : 0;
                b = prev[i];
                c = (i >= chn) ? prev[i - chn] : 0;
                if (flt == 1) cur[i] = (unsigned char)(cur[i] + a);
                else if (flt == 2) cur[i] = (unsigned char)(cur[i] + b);
                else if (flt == 3)
                    cur[i] = (unsigned char)(cur[i] + ((a + b) >> 1));
                else if (flt == 4) {
                    int p = (int)a + (int)b - (int)c;
                    int pa = abs(p - (int)a), pb = abs(p - (int)b);
                    int pc = abs(p - (int)c);
                    unsigned pr = (pa <= pb && pa <= pc) ? a
                        : (pb <= pc)                     ? b
                                                        : c;
                    cur[i] = (unsigned char)(cur[i] + pr);
                } else if (flt != 0) {
                    img_free(im);
                    im = NULL;
                    goto out;
                }
            }
            for (x = 0; x < w; x++) {
                unsigned r, g, bl;
                if (ct == 0) {
                    r = g = bl = cur[x];
                } else {
                    r = cur[x * chn];
                    g = cur[x * chn + 1];
                    bl = cur[x * chn + 2];
                }
                im->px[y * w + x] = (r << 16) | (g << 8) | bl;
            }
            memcpy(prev, cur, stride);
        }
        free(prev);
        free(cur);
    }
out:
    free(idat);
    free(raw);
    if (!done && im) { img_free(im); im = NULL; }
    return im;
}
