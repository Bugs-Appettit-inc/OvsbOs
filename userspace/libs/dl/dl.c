/* dl.c — libdl mínima sobre o kernel OvsbMk
 *
 * Usa o syscall 300 (SYS_dlopen) que chama dynlink_process() do kernel
 * para carregar .so (DT_NEEDED + relocações RELATIVE/GLOB_DAT/JUMP_SLOT/
 * COPY/IRELATIVE via dynlink.c).
 */

#include "dl.h"
#include <stddef.h>
#include <stdint.h>

static inline long sys_dlopen(const char *path) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(300), "D"(path)
        : "rcx", "r11", "memory"
    );
    return ret;
}

void *dlopen(const char *path) {
    long ret = sys_dlopen(path);
    if (ret < 0) return NULL;
    /* O kernel carrega em DL_LIB_BASE = 0x48000000 (sys_dlopen). */
    return (void *)(uintptr_t)0x48000000ULL;
}

void *dlsym(void *handle, const char *name) {
    (void)name;
    /* Implementação mínima: devolve o handle (base). A resolução por nome
     * exige parsing da tabela de símbolos do .so no userland — deferido. */
    return handle;
}

int dlclose(void *handle) {
    (void)handle;
    return 0;
}
