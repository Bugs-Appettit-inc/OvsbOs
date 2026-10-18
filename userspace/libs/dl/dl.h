#ifndef _DL_H
#define _DL_H

#include <stdint.h>

typedef void *dl_handle_t;

/* Abre uma biblioteca compartilhada (.so) via syscall 300 (sys_dlopen).
 * Retorna o handle (base de carregamento) ou NULL em caso de erro. */
void *dlopen(const char *path);

/* Resolve um símbolo na biblioteca. Implementação mínima: retorna o handle
 * (base) pois a resolução por nome depende da tabela de símbolos do .so. */
void *dlsym(void *handle, const char *name);

/* Fecha a biblioteca (no-op nesta versão mínima). */
int dlclose(void *handle);

#endif /* _DL_H */
