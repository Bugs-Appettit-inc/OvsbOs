/* wayland_lib_test — valida libwayland-client no OvsbMk
 *
 * Conecta ao compositor via libwayland-client (wl_display_connect),
 * faz bind de wl_compositor pelo registry, cria uma wl_surface e
 * comita. Sucesso = imprime "wayland_lib_test OK" e encerra com rc=0.
 *
 * Compilar: gcc -static -I wl-build/wayland/include ... libwayland-client.a
 *             libwayland-util.a libffi.a -lpthread -lrt -ldl -lm
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

static struct wl_compositor *g_comp = NULL;
static struct wl_surface    *g_surf = NULL;
static int g_done = 0;

static void registry_global(void *data, struct wl_registry *reg,
                            uint32_t name, const char *iface, uint32_t ver){
    (void)data; (void)ver;
    if (strcmp(iface, "wl_compositor") == 0) {
        g_comp = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
        printf("[WL-LIB-TEST] bound wl_compositor name=%u\n", name);
    }
}
static void registry_global_remove(void *data, struct wl_registry *reg,
                                   uint32_t name){
    (void)data; (void)reg; (void)name;
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(void){
    const char *path = getenv("WAYLAND_DISPLAY");
    if (!path) path = "wayland-0";

    struct wl_display *d = wl_display_connect(path);
    if (!d) {
        printf("[WL-LIB-TEST] wl_display_connect(%s) FAIL\n", path);
        return 1;
    }
    printf("[WL-LIB-TEST] connected to %s (fd=%d)\n",
           path, wl_display_get_fd(d));

    struct wl_registry *reg = wl_display_get_registry(d);
    wl_registry_add_listener(reg, &registry_listener, NULL);

    /* roundtrip pra receber globals + bind */
    if (wl_display_roundtrip(d) < 0) {
        printf("[WL-LIB-TEST] roundtrip FAIL\n");
        return 1;
    }

    if (!g_comp) {
        printf("[WL-LIB-TEST] FAIL: wl_compositor global not found\n");
        return 1;
    }

    g_surf = wl_compositor_create_surface(g_comp);
    if (!g_surf) {
        printf("[WL-LIB-TEST] FAIL: create_surface\n");
        return 1;
    }
    printf("[WL-LIB-TEST] wl_surface created\n");

    wl_surface_commit(g_surf);
    printf("[WL-LIB-TEST] wl_surface.commit\n");

    /* mais um roundtrip pra garantir entrega do commit */
    wl_display_roundtrip(d);

    wl_surface_destroy(g_surf);
    wl_compositor_destroy(g_comp);
    wl_registry_destroy(reg);
    wl_display_disconnect(d);

    printf("[WL-LIB-TEST] OK\n");
    g_done = 1;
    return 0;
}
