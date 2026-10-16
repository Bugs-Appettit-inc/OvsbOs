/* oep.h — Ovsb Executable Program (.oep), formato v1.
 * Layout: header fixo de 128 bytes, seguido do ELF cru.
 * Compartilhado entre ferramentas host/inside e (futuro) kernel. */
#ifndef OVSB_OEP_H
#define OVSB_OEP_H

#include <stdint.h>

#define OEP_MAGIC "OEP"        /* magic[3] = '\0' */
#define OEP_VERSION    1u
#define OEP_HEADER_SIZE 128u

struct oep_header {
    char     magic[4];        /* "OEP\0"                              */
    uint16_t version;         /* 1                                    */
    uint16_t flags;           /* reservado (0)                        */
    uint32_t elf_offset;      /* onde o ELF começa (128)              */
    uint32_t elf_size;        /* tamanho do ELF                       */
    char     name[32];        /* nome do app (UTF-8, NUL-terminated)  */
    char     category[16];    /* "games", "tools", "graphics", ...    */
    uint8_t  hash[32];        /* SHA-256 do ELF (integridade)         */
    uint8_t  reserved[20];    /* reservado p/ futuro (0)              */
}; /* sizeof == 128 */

static inline unsigned oep_header_size(void) { return OEP_HEADER_SIZE; }

#endif /* OVSB_OEP_H */
