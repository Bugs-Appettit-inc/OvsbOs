/* drm.c — /dev/dri/card0 (DRM-like) para OvsbOS + Mesa virgl.
 * Escopo honesto: VERSION, GET_CAP, dumb buffers (create/map/destroy),
 * topologia mínima (1 CRTC/conector/encoder 1280x800 ou real), ATOMIC
 * no-op. VIRTGPU_* 3D (EXECBUFFER etc.): ENOSYS (bloqueado na Mesa).
 * SEM SIMD. Estilo do codebase: dereferencia user ptrs direto.
 */
#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "process.h"
#include "vm_map.h"
#include "../lib/gui/vesa.h"

extern void serial_puts(const char *s);
extern void serial_puthex(uint32_t v);
extern framebuffer_t g_fb;
extern int g_virtio_active;

#define DRM_IOCTL_VERSION        0x6400
#define DRM_IOCTL_GET_CAP        0x640C
#define DRM_IOCTL_MODE_GETRESOURCES 0x64A0
#define DRM_IOCTL_MODE_GETCRTC   0x64A1
#define DRM_IOCTL_MODE_GETENCODER 0x64A6
#define DRM_IOCTL_MODE_GETCONNECTOR 0x64A7
#define DRM_IOCTL_MODE_CREATE_DUMB 0x64B2
#define DRM_IOCTL_MODE_MAP_DUMB 0x64B3
#define DRM_IOCTL_MODE_DESTROY_DUMB 0x64B4
#define DRM_IOCTL_MODE_ATOMIC   0x64BC

#define DRM_CAP_DUMB_BUFFER      0x1
#define DRM_CAP_DUMB_PREFERRED_DEPTH 0x3
#define DRM_CAP_DUMB_PREFER_SHADOW 0x4
#define DRM_CAP_TIMESTAMP_MONOTONIC 0x6

#define DRM_MAX_DUMB 16
#define DRM_DUMB_OFF_BASE 0xD0000000ULL

typedef struct {
    int used;
    uint32_t handle;
    uint64_t addr;   /* page-aligned (para mapear) */
    void *raw;       /* ponteiro original p/ kfree */
    uint64_t size;
    uint32_t pitch;
    uint32_t width, height;
} drm_dumb_t;

static drm_dumb_t g_dumb[DRM_MAX_DUMB];
static uint32_t g_dumb_next = 1;

struct drm_version_s {
    int version_major, version_minor, version_patchlevel;
    uint64_t name_len; char *name;
    uint64_t date_len; char *date;
    uint64_t desc_len; char *desc;
};

struct drm_get_cap_s {
    uint64_t capability;
    uint64_t value;
};

struct drm_mode_create_dumb_s {
    uint32_t height, width, flags;
    uint32_t handle, pitch;
    uint64_t size;
};

struct drm_mode_map_dumb_s {
    uint32_t handle, pad;
    uint64_t offset;
};

struct drm_mode_destroy_dumb_s {
    uint32_t handle;
};

struct drm_mode_get_resources_s {
    uint64_t fbs, crtcs, connectors, encoders;
    uint32_t count_fbs, count_crtcs, count_connectors, count_encoders;
    uint32_t min_width, max_width, min_height, max_height;
};

struct drm_mode_crtc_s {
    uint32_t crtc_id, fb_id;
    uint32_t x, y;
    uint64_t mode_valid;
    uint32_t width, height;
};

struct drm_mode_get_connector_s {
    uint32_t connector_id, encoder_id, connector_type, connector_type_id;
    uint32_t connection, mm_width, mm_height, subpixel;
    uint64_t modes_ptr;
    int count_modes, count_props, count_encoders;
};

static void wstr(char *d, const char *s, uint64_t max) {
    uint64_t i = 0;
    if (!d || !max) return;
    while (i + 1 < max && s[i]) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static uint32_t disp_w(void) {
    return g_fb.width ? g_fb.width : 1280;
}

static uint32_t disp_h(void) {
    return g_fb.height ? g_fb.height : 800;
}

/* Retorna 0 ok, -38 (ENOSYS) não implementado, -14 (EFAULT) ponteiro ruim.
 * (EFAULT só se arg==0; validação profunda não existe neste kernel.) */
int drm_ioctl(unsigned long req, void *argp) {
    if (!argp) return -14;
    switch (req) {
    case DRM_IOCTL_VERSION: {
        struct drm_version_s *v = argp;
        v->version_major = 1;
        v->version_minor = 0;
        v->version_patchlevel = 0;
        wstr(v->name, "virgl", v->name_len);
        wstr(v->date, "20261016", v->date_len);
        wstr(v->desc, "OvsbOS VirGL DRM", v->desc_len);
        return 0;
    }
    case DRM_IOCTL_GET_CAP: {
        struct drm_get_cap_s *c = argp;
        switch (c->capability) {
        case DRM_CAP_DUMB_BUFFER: c->value = 1; return 0;
        case DRM_CAP_DUMB_PREFERRED_DEPTH: c->value = 32; return 0;
        case DRM_CAP_DUMB_PREFER_SHADOW: c->value = 1; return 0;
        case DRM_CAP_TIMESTAMP_MONOTONIC: c->value = 1; return 0;
        default: c->value = 0; return -22;
        }
    }
    case DRM_IOCTL_MODE_CREATE_DUMB: {
        struct drm_mode_create_dumb_s *d = argp;
        int i, h = -1;
        uint64_t sz;
        void *raw;
        uint64_t base;
        if (!d->width || !d->height || d->width > 4096 || d->height > 4096)
            return -22;
        for (i = 0; i < DRM_MAX_DUMB; i++)
            if (!g_dumb[i].used) { h = i; break; }
        if (h < 0) return -12;
        sz = (uint64_t)d->width * d->height * 4;
        sz = (sz + 0xFFF) & ~0xFFFULL;
        /* pml4_map_phys só cria PDEs de 2MB (sem checar!): o buffer
         * PRECISA ser 2MB-alinhado e múltiplo de 2MB, senão os bits
         * 12-20 viram reservados e dá #PF RSVD no primeiro toque.
         * kmalloc>=2048 usa buddy (contíguo+alinhado à ordem). */
        {
            uint64_t sz2 = (sz + 0x1FFFFFULL) & ~0x1FFFFFULL;
            void *raw = kmalloc((size_t)sz2);
            uint64_t base;
            uint64_t o;
            if (!raw) return -12;
            base = (uint64_t)(uintptr_t)raw;
            if (base & 0x1FFFFFULL) {
                kfree(raw);
                return -12;
            }
            for (o = 0; o < sz; o += 0x1000)
                ((volatile uint8_t *)(uintptr_t)(base + o))[0] = 0;
            g_dumb[h].used = 1;
            g_dumb[h].handle = g_dumb_next;
            g_dumb[h].addr = base;
            g_dumb[h].raw = raw;
            g_dumb[h].size = sz;
            g_dumb[h].pitch = d->width * 4;
            g_dumb[h].width = d->width;
            g_dumb[h].height = d->height;
            d->handle = g_dumb_next;
            if (++g_dumb_next > 0xFFFF) g_dumb_next = 1;
            d->pitch = g_dumb[h].pitch;
            d->size = sz;
        }
        return 0;
    }
    case DRM_IOCTL_MODE_MAP_DUMB: {
        struct drm_mode_map_dumb_s *m = argp;
        int i;
        if (!m->handle) return -22;
        for (i = 0; i < DRM_MAX_DUMB; i++) {
            if (g_dumb[i].used && g_dumb[i].handle == m->handle) {
                m->offset = DRM_DUMB_OFF_BASE + ((uint64_t)i << 12);
                return 0;
            }
        }
        return -22;
    }
    case DRM_IOCTL_MODE_DESTROY_DUMB: {
        struct drm_mode_destroy_dumb_s *d = argp;
        int i;
        if (!d->handle) return -22;
        for (i = 0; i < DRM_MAX_DUMB; i++) {
            if (g_dumb[i].used && g_dumb[i].handle == d->handle) {
                g_dumb[i].used = 0;
                kfree(g_dumb[i].raw);
                g_dumb[i].raw = 0;
                return 0;
            }
        }
        return -22;
    }
    case DRM_IOCTL_MODE_GETRESOURCES: {
        struct drm_mode_get_resources_s *r = argp;
        r->count_fbs = 0;
        r->count_crtcs = 1;
        r->count_connectors = 1;
        r->count_encoders = 1;
        if (r->crtcs) *(uint32_t *)(uintptr_t)r->crtcs = 1;
        if (r->connectors) *(uint32_t *)(uintptr_t)r->connectors = 2;
        if (r->encoders) *(uint32_t *)(uintptr_t)r->encoders = 3;
        r->min_width = 320;
        r->max_width = disp_w();
        r->min_height = 200;
        r->max_height = disp_h();
        return 0;
    }
    case DRM_IOCTL_MODE_GETCRTC: {
        struct drm_mode_crtc_s *c = argp;
        c->crtc_id = 1;
        c->fb_id = 0;
        c->x = 0;
        c->y = 0;
        c->mode_valid = 1;
        c->width = disp_w();
        c->height = disp_h();
        return 0;
    }
    case DRM_IOCTL_MODE_GETCONNECTOR: {
        struct drm_mode_get_connector_s *c = argp;
        c->connector_id = 2;
        c->encoder_id = 3;
        c->connector_type = 0;
        c->connector_type_id = 0;
        c->connection = 1;
        c->mm_width = 340;
        c->mm_height = 190;
        c->subpixel = 0;
        c->count_modes = 1;
        return 0;
    }
    case DRM_IOCTL_MODE_GETENCODER: {
        /* struct drm_mode_get_encoder: encoder_id, encoder_type, crtc_id */
        uint32_t *e = argp;
        e[0] = 3;
        e[1] = 0;
        e[2] = 1;
        return 0;
    }
    case DRM_IOCTL_MODE_ATOMIC: {
        /* ATOMIC no-op: display já está setado (scanout fixo).
         * Modeset real precisaria programar CRTC — documentado. */
        return 0;
    }
    default:
        break;
    }
    /* VIRTGPU_* (0x6440+) e resto: ENOSYS honesto (3D bloqueado). */
    serial_puts("[DRM] ENOSYS req=");
    serial_puthex((uint32_t)req);
    serial_puts("\r\n");
    return -38;
}

/* Mapeia dumb buffer no processo (chamado do mmap com offset cookie).
 * VA determinística por slot (0x60000000+16MB) + recusa se vm_map já
 * cobre (sem tracking duplo, como o path do fb0). Retorna VA ou 0. */
uint64_t drm_mmap_dumb(uint64_t pml4, uint64_t off, uint64_t len) {
    int i;
    uint64_t va;
    if (off < DRM_DUMB_OFF_BASE) return 0;
    i = (int)((off - DRM_DUMB_OFF_BASE) >> 12);
    if (i < 0 || i >= DRM_MAX_DUMB || !g_dumb[i].used) return 0;
    if (!len || len > g_dumb[i].size) len = g_dumb[i].size;
    va = 0x60000000ULL + (uint64_t)i * 0x1000000ULL;
    {
        struct pcb *cur = process_current();
        if (!cur) return 0;
        if (vm_map_covers(cur->vm_map, va)) return 0;
        {
            extern int pml4_map_phys(uint64_t, uint64_t, uint64_t, uint64_t,
                                     int);
            uint64_t sz = (len + 0xFFF) & ~0xFFFULL;
            if (pml4_map_phys(pml4, va, g_dumb[i].addr, sz, 1) < 0)
                return 0;
        }
        {
            uint64_t o;
            for (o = 0; o < len; o += 0x1000)
                ((volatile uint8_t *)(uintptr_t)(va + o))[0] = 0;
        }
    }
    return va;
}
