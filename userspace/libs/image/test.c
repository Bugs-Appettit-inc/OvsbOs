/* imgtest — smoke test da lib: roundtrip BMP/PPM + load PNG + ops. */
#include <stdio.h>
#include <string.h>
#include "image.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL: %s\n", msg); fails++; } \
    else printf("ok: %s\n", msg); } while (0)

int main(int argc, char **argv) {
    img_t *a, *b, *c;
    if (argc < 2) {
        printf("uso: imgtest BASE   (gera base.bmp/ppm, testa roundtrip)\n");
        return 1;
    }
    a = img_new(37, 23);
    CHECK(a, "new");
    {
        int x, y;
        for (y = 0; y < 37 - 14; y++)
            for (x = 0; x < 37; x++)
                a->px[y * 37 + x] =
                    ((unsigned)(x * 7) << 16) | ((unsigned)(y * 11) << 8) |
                    (unsigned)((x + y) * 3);
    }
    CHECK(img_save(a, "t_bmp.bmp") == 0, "save bmp");
    CHECK(img_save(a, "t_ppm.ppm") == 0, "save ppm");
    b = img_load("t_bmp.bmp");
    CHECK(b && b->w == 37 && b->h == 23, "load bmp dims");
    CHECK(b && memcmp(a->px, b->px, 37U * 23U * 4) == 0, "bmp roundtrip");
    c = img_load("t_ppm.ppm");
    CHECK(c && memcmp(a->px, c->px, 37U * 23U * 4) == 0, "ppm roundtrip");
    {
        img_t *s = img_scale(a, 10, 10);
        CHECK(s && s->w == 10 && s->h == 10, "scale");
        img_free(s);
    }
    {
        img_t *k = img_crop(a, 5, 5, 10, 10);
        CHECK(k && k->w == 10 && k->h == 10 &&
              k->px[0] == a->px[5 * 37 + 5], "crop");
        img_free(k);
    }
    {
        img_t *d = img_new(37, 23);
        img_blend(d, a, 0, 0, 255);
        CHECK(memcmp(d->px, a->px, 37U * 23U * 4) == 0, "blend alpha=255");
        img_free(d);
    }
    if (argv[1][0]) {
        img_t *p = img_load(argv[1]);
        CHECK(p != NULL, "load png real");
        if (p) {
            printf("png: %dx%d px[0]=%06x\n", p->w, p->h, p->px[0]);
            img_free(p);
        }
    }
    img_free(a);
    img_free(b);
    img_free(c);
    printf(fails ? "RESULT: FAIL\n" : "RESULT: PASS\n");
    return fails ? 1 : 0;
}
