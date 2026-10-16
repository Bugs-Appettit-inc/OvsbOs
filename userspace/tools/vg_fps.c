/* vg_fps — benchmark 2D no framebuffer OvsbOS (virtio-gpu ou VESA).
 * Retângulo se movendo + flush por frame; FPS no stdout (serial) a cada s.
 * Uso: VG_FPS [segundos] (default 10). Syscalls TipOS raw via syscall().
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>

static long ovsb(long n, long a, long b, long c, long d) {
    register long r10 __asm__("r10") = d;
    long r;
    __asm__ volatile("syscall"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10)
                     : "rcx", "r11", "memory");
    return r;
}

static unsigned long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000ULL +
           (unsigned long long)ts.tv_nsec / 1000000ULL;
}

int main(int argc, char **argv) {
    unsigned long long addr = 0, w = 0, h = 0, pitch = 0;
    int secs = argc > 1 ? atoi(argv[1]) : 10;
    uint32_t *fb;
    unsigned long stride;
    unsigned long long t0, last, deadline;
    long frames = 0, total = 0;
    int x = 0, dx = 4;
    if (ovsb(200, (long)&addr, (long)&w, (long)&h, (long)&pitch) != 0 ||
        !addr || !w || !h) {
        printf("vg_fps: sem framebuffer\n");
        return 1;
    }
    fb = (uint32_t *)(uintptr_t)addr;
    stride = pitch / 4;
    printf("vg_fps: %llux%llu stride=%llu\n", w, h,
           (unsigned long long)stride);
    t0 = last = now_ms();
    deadline = t0 + (unsigned long long)secs * 1000ULL;
    while (now_ms() < deadline) {
        unsigned long y, i;
        uint32_t rw = w > 120 ? 120 : (uint32_t)w / 2;
        uint32_t rh = h > 90 ? 90 : (uint32_t)h / 2;
        unsigned long yy = (h - rh) / 2;
        for (y = 0; y < h; y++)
            for (i = 0; i < w; i++)
                fb[y * stride + i] = 0xFF101418;
        for (y = 0; y < rh; y++)
            for (i = 0; i < rw; i++)
                fb[(yy + y) * stride + (unsigned long)x + i] = 0xFFC00000;
        ovsb(201, 0, 0, 0, 0);
        frames++;
        total++;
        x += dx;
        if (x + (int)rw >= (int)w || x <= 0) dx = -dx;
        if (now_ms() - last >= 1000ULL) {
            printf("vg_fps: %ld fps\n", frames);
            frames = 0;
            last = now_ms();
        }
    }
    printf("vg_fps: fim total=%ld\n", total);
    return 0;
}
