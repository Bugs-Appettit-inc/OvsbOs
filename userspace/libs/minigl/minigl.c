/* minigl.c — OpenGL-like API mínima para OvsbOS
 *
 * Implementa um subconjunto muito reduzido de OpenGL 1.1 sobre software
 * rendering, usando dumb buffers do DRM (/dev/dri/card0) do kernel como
 * superfície de destino. O resultado é copiado para o framebuffer do
 * compositor Wayland via minigl_swap_buffers().
 *
 * Este NÃO é um OpenGL completo. Falta: shaders, texturas, depth buffer,
 * blending, matrices, etc. Serve como base para começar a portar
 * aplicações gráficas e, eventualmente, substituir por Mesa real.
 */

#define _GNU_SOURCE
#include "minigl.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/* ── DRM ioctls (mesmo do kernel OvsbOS/kernel/drm.c) ── */
#define DRM_IOCTL_VERSION        0x6400
#define DRM_IOCTL_GET_CAP        0x640C
#define DRM_IOCTL_MODE_GETRESOURCES 0x64A0
#define DRM_IOCTL_MODE_GETCRTC   0x64A1
#define DRM_IOCTL_MODE_GETENCODER 0x64A6
#define DRM_IOCTL_MODE_GETCONNECTOR 0x64A7
#define DRM_IOCTL_MODE_CREATE_DUMB 0x64B2
#define DRM_IOCTL_MODE_MAP_DUMB    0x64B3
#define DRM_IOCTL_MODE_DESTROY_DUMB 0x64B4

/* Estruturas DRM (simplificadas, alinhadas com o kernel) */
struct drm_mode_create_dumb {
    uint32_t height;
    uint32_t width;
    uint32_t flags;
    uint32_t handle;
    uint32_t pitch;
    uint64_t size;
};

struct drm_mode_map_dumb {
    uint32_t handle;
    uint32_t pad;
    uint64_t offset;
};

struct drm_mode_destroy_dumb {
    uint32_t handle;
};

/* ── Syscalls diretas ── */
static inline int sys_open(const char *path, int flags, int mode) {
    int ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(2), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory");
    return ret;
}
static inline int sys_close(int fd) {
    int ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(3), "D"(fd)
        : "rcx", "r11", "memory");
    return ret;
}
static inline int sys_ioctl(int fd, unsigned long request, void *arg) {
    int ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(16), "D"(fd), "S"(request), "d"(arg)
        : "rcx", "r11", "memory");
    return ret;
}
static inline void *sys_mmap(void *addr, size_t len, int prot, int flags,
                             int fd, off_t off) {
    void *ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(9), "D"(addr), "S"(len), "d"(prot), "r"(flags), "r"(fd), "r"(off)
        : "rcx", "r11", "r8", "r9", "memory");
    return ret;
}
static inline int sys_munmap(void *addr, size_t len) {
    int ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(11), "D"(addr), "S"(len)
        : "rcx", "r11", "memory");
    return ret;
}

/* ── Estado global ── */
static int   g_drm_fd = -1;
static void *g_fb     = NULL;   /* mapeamento do dumb buffer */
static int   g_width  = 0;
static int   g_height = 0;
static int   g_pitch  = 0;

/* ── Estado do pipeline fixo simplificado ── */
static float g_clear_r = 0.0f, g_clear_g = 0.0f, g_clear_b = 0.0f, g_clear_a = 1.0f;
static float g_cur_r  = 1.0f, g_cur_g  = 1.0f, g_cur_b  = 1.0f;
static int   g_vp_x   = 0, g_vp_y   = 0, g_vp_w  = 0, g_vp_h  = 0;
static int   g_in_begin = 0;

/* ── Helpers de pixel ── */

static inline uint32_t float_to_unorm8(float f) {
    if (f <= 0.0f) return 0;
    if (f >= 1.0f) return 255;
    return (uint32_t)(f * 255.0f + 0.5f);
}

static inline void set_pixel(int x, int y, float r, float g, float b, float a) {
    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return;
    uint8_t *p = (uint8_t *)g_fb + y * g_pitch + x * 4;
    p[0] = float_to_unorm8(b);   /* BGRA */
    p[1] = float_to_unorm8(g);
    p[2] = float_to_unorm8(r);
    p[3] = float_to_unorm8(a);
}

/* ── API pública ── */

void glClearColor(float r, float g, float b, float a) {
    g_clear_r = r; g_clear_g = g; g_clear_b = b; g_clear_a = a;
}

void glClear(unsigned int mask) {
    if (!(mask & GL_COLOR_BUFFER_BIT)) return;
    uint32_t c = (float_to_unorm8(g_clear_b) << 0) |
                 (float_to_unorm8(g_clear_g) << 8) |
                 (float_to_unorm8(g_clear_r) << 16) |
                 (float_to_unorm8(g_clear_a) << 24);
    int count = g_width * g_height;
    uint32_t *p = (uint32_t *)g_fb;
    for (int i = 0; i < count; i++) p[i] = c;
}

void glViewport(int x, int y, int w, int h) {
    g_vp_x = x; g_vp_y = y; g_vp_w = w; g_vp_h = h;
}

void glColor3f(float r, float g, float b) {
    g_cur_r = r; g_cur_g = g; g_cur_b = b;
}

void glVertex2f(float x, float y) {
    if (!g_in_begin) return;
    /* Converte coordenadas normalizadas (-1..1) para pixels do viewport */
    int px = g_vp_x + (int)((x + 1.0f) * 0.5f * g_vp_w);
    int py = g_vp_y + (int)((1.0f - y) * 0.5f * g_vp_h);
    set_pixel(px, py, g_cur_r, g_cur_g, g_cur_b, 1.0f);
}

void glBegin(unsigned int mode) {
    (void)mode;
    g_in_begin = 1;
}

void glEnd(void) {
    g_in_begin = 0;
}

void glFlush(void) {
    /* Sem pipeline para esvaziar em software rendering simples */
}

int minigl_init(int width, int height, void *fb_addr) {
    (void)fb_addr;
    g_width  = width;
    g_height = height;

    g_drm_fd = sys_open("/dev/dri/card0", 2, 0); /* O_RDWR */
    if (g_drm_fd < 0) {
        fprintf(stderr, "[minigl] não pôde abrir /dev/dri/card0\n");
        return -1;
    }

    /* Cria dumb buffer */
    struct drm_mode_create_dumb req;
    memset(&req, 0, sizeof(req));
    req.height = height;
    req.width  = width;
    int iret = sys_ioctl(g_drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &req);
    fprintf(stderr, "[minigl] CREATE_DUMB ret=%d h=%u w=%u\n", iret, req.height, req.width);
    if (iret < 0) {
        fprintf(stderr, "[minigl] CREATE_DUMB falhou (errno=%d)\n", -iret);
        return -1;
    }
    fprintf(stderr, "[minigl] CREATE_DUMB ok: handle=%u pitch=%u size=%llu\n",
            req.handle, req.pitch, (unsigned long long)req.size);
    g_pitch = req.pitch;

    /* Mapeia o buffer */
    struct drm_mode_map_dumb mapreq;
    memset(&mapreq, 0, sizeof(mapreq));
    mapreq.handle = req.handle;
    if (sys_ioctl(g_drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mapreq) < 0) {
        fprintf(stderr, "[minigl] MAP_DUMB falhou\n");
        return -1;
    }

    g_fb = sys_mmap(NULL, req.size, 3 /* PROT_READ|WRITE */,
                    1 /* MAP_SHARED */, g_drm_fd, mapreq.offset);
    if (g_fb == (void *)-1) {
        fprintf(stderr, "[minigl] mmap falhou\n");
        return -1;
    }

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    fprintf(stderr, "[minigl] iniciado %dx%d pitch=%d\n", width, height, g_pitch);
    return 0;
}

void minigl_shutdown(void) {
    if (g_fb && g_fb != (void *)-1) {
        sys_munmap(g_fb, (size_t)(g_height * g_pitch));
        g_fb = NULL;
    }
    if (g_drm_fd >= 0) {
        sys_close(g_drm_fd);
        g_drm_fd = -1;
    }
}

void minigl_swap_buffers(void) {
    /* Em software rendering simples, o dumb buffer já é o framebuffer visível.
       Se o compositor Wayland usa um fb separado, copiaria aqui. */
    glFlush();
}

void *minigl_get_fb(void) { return g_fb; }
int  minigl_get_width(void)  { return g_width; }
int  minigl_get_height(void) { return g_height; }
