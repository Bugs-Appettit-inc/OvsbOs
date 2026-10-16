/* drmtest — exercita /dev/dri/card0: version, caps, dumb, resources. */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

struct dv { int mj, mn, pl; unsigned long nl; char *n; unsigned long dl; char *d; unsigned long sl; char *s; };
struct dc { unsigned long cap, val; };
struct dd { unsigned w, h, f, hnd, pitch; unsigned long sz; };
struct dm { unsigned hnd, pad; unsigned long off; };
struct dx { unsigned hnd; };
struct dr { unsigned long f, c, co, e; unsigned cf, cc, cn, ce; unsigned mnw, mxw, mnh, mxh; };

int main(void) {
    int fd = open("/dev/dri/card0", O_RDWR);
    int fails = 0;
    struct dv v;
    struct dc c;
    struct dd d;
    struct dm m;
    struct dr r;
    if (fd < 0) { printf("drmtest: sem card0\n"); return 1; }
    memset(&v, 0, sizeof(v));
    v.nl = 16;
    v.n = malloc(16);
    v.dl = 16;
    v.d = malloc(16);
    v.sl = 32;
    v.s = malloc(32);
    if (ioctl(fd, 0x6400, &v) != 0) { printf("FAIL version\n"); fails++; }
    else printf("drm: %s %d.%d.%d (%s)\n", v.n, v.mj, v.mn, v.pl, v.s);
    c.cap = 1;
    c.val = 0;
    if (ioctl(fd, 0x640C, &c) != 0 || !c.val) {
        printf("FAIL dumb-cap\n");
        fails++;
    } else printf("dumb: ok\n");
    memset(&d, 0, sizeof(d));
    d.w = 64;
    d.h = 64;
    if (ioctl(fd, 0x64B2, &d) != 0 || !d.hnd) {
        printf("FAIL create\n");
        fails++;
    } else printf("dumb: %ux%u h=%u pitch=%u size=%lu\n", d.w, d.h, d.hnd,
                  d.pitch, d.sz);
    memset(&m, 0, sizeof(m));
    m.hnd = d.hnd;
    if (ioctl(fd, 0x64B3, &m) != 0 || !m.off) {
        printf("FAIL map\n");
        fails++;
    } else {
        void *p = mmap(0, d.sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                       m.off);
        printf("mmap: %p (off %lx)\n", p, m.off);
        if (p == MAP_FAILED) {
            printf("FAIL mmap\n");
            fails++;
        } else {
            ((volatile unsigned *)p)[0] = 0x12345678;
            ((volatile unsigned *)p)[100] = 0xDEADBEEF;
            if (((volatile unsigned *)p)[0] != 0x12345678 ||
                ((volatile unsigned *)p)[100] != 0xDEADBEEF) {
                printf("FAIL readback\n");
                fails++;
            } else printf("readback: ok\n");
        }
    }
    {
        struct dx x;
        x.hnd = d.hnd;
        if (ioctl(fd, 0x64B4, &x) != 0) { printf("FAIL destroy\n"); fails++; }
        else printf("destroy: ok\n");
    }
    memset(&r, 0, sizeof(r));
    if (ioctl(fd, 0x64A0, &r) != 0) { printf("FAIL resources\n"); fails++; }
    else printf("res: crtc=%u conn=%u enc=%u max=%ux%u\n", r.cc, r.cn, r.ce,
                r.mxw, r.mxh);
    close(fd);
    printf(fails ? "RESULT: FAIL\n" : "RESULT: PASS\n");
    return fails ? 1 : 0;
}
