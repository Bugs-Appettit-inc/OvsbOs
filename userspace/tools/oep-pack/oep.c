/* oep.c — ferramentas .oep (empacotar/listar/extrair/rodar/verificar).
 * Formato v1: header fixo de 128 (userspace/libs/oep/oep.h) + ELF cru
 * em elf_offset. Multi-call por argv0: oep-pack / oep-info / oep-unpack
 * / oep-run.
 * Uso: oep-pack APP.elf NOME CATEGORIA -o APP.oep
 *      oep-info APP.oep | oep-unpack APP.oep DIR/ | oep-run APP.oep [args]
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "../../libs/oep/oep.h"
#include "../../libs/oep/sha256.h"

static const char *base(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* ---- header helpers (endianness LE, alinhado) ---- */
static uint16_t r16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t r32l(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Lê e valida o header; f posicionado em 0. Retorna 0 ok. */
static int read_header(FILE *f, struct oep_header *h) {
    uint8_t raw[OEP_HEADER_SIZE];
    if (fseek(f, 0, SEEK_SET) != 0) return -1;
    if (fread(raw, 1, OEP_HEADER_SIZE, f) != OEP_HEADER_SIZE) return -1;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, raw, 4);
    if (memcmp(h->magic, OEP_MAGIC, 4) != 0) return -1;
    h->version    = r16(raw + 4);
    h->flags      = r16(raw + 6);
    h->elf_offset = r32l(raw + 8);
    h->elf_size   = r32l(raw + 12);
    memcpy(h->name, raw + 16, 32);
    memcpy(h->category, raw + 48, 16);
    memcpy(h->hash, raw + 64, 32);
    if (h->version != OEP_VERSION) return -1;
    if (h->elf_offset < OEP_HEADER_SIZE) return -1;
    /* name/category NUL-terminated dentro dos campos */
    h->name[31] = 0; h->category[15] = 0;
    return 0;
}

/* ---- pack ---- */
static int do_pack(const char *elf, const char *name, const char *category,
                   const char *out) {
    FILE *f = fopen(elf, "rb"), *o;
    uint8_t *data;
    long sz;
    struct oep_header h;
    sha256_ctx sc;
    uint8_t raw[OEP_HEADER_SIZE];
    size_t nl, cl;

    if (!f) { fprintf(stderr, "oep-pack: %s: %s\n", elf, strerror(errno)); return 1; }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 64) { fprintf(stderr, "oep-pack: %s: muito pequeno\n", elf); fclose(f); return 1; }
    data = malloc((size_t)sz);
    if (!data || fread(data, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "oep-pack: leitura falhou\n");
        if (data) free(data);
        fclose(f);
        return 1;
    }
    fclose(f);
    if (memcmp(data, "\x7f" "ELF", 4) != 0)
        fprintf(stderr, "oep-pack: aviso: %s nao parece ELF\n", elf);

    memset(&h, 0, sizeof(h));
    memcpy(h.magic, OEP_MAGIC, 4);
    h.version = OEP_VERSION;
    h.flags = 0;
    h.elf_offset = OEP_HEADER_SIZE;
    h.elf_size = (uint32_t)sz;
    nl = strlen(name);   if (nl >= sizeof(h.name))     nl = sizeof(h.name) - 1;
    cl = strlen(category); if (cl >= sizeof(h.category)) cl = sizeof(h.category) - 1;
    memcpy(h.name, name, nl);
    memcpy(h.category, category, cl);
    sha256_init(&sc);
    sha256_update(&sc, data, (size_t)sz);
    sha256_final(&sc, h.hash);

    memset(raw, 0, sizeof(raw));
    memcpy(raw, h.magic, 4);
    raw[4] = (uint8_t)(h.version & 0xFF); raw[5] = (uint8_t)(h.version >> 8);
    raw[8]  = (uint8_t)h.elf_offset; raw[9] = (uint8_t)(h.elf_offset >> 8);
    raw[10] = (uint8_t)(h.elf_offset >> 16); raw[11] = (uint8_t)(h.elf_offset >> 24);
    raw[12] = (uint8_t)h.elf_size; raw[13] = (uint8_t)(h.elf_size >> 8);
    raw[14] = (uint8_t)(h.elf_size >> 16); raw[15] = (uint8_t)(h.elf_size >> 24);
    memcpy(raw + 16, h.name, nl + (nl < sizeof(h.name) ? 1 : 0));
    memcpy(raw + 48, h.category, cl + (cl < sizeof(h.category) ? 1 : 0));
    memcpy(raw + 64, h.hash, 32);

    o = fopen(out, "wb");
    if (!o) { fprintf(stderr, "oep-pack: %s: %s\n", out, strerror(errno)); free(data); return 1; }
    if (fwrite(raw, 1, OEP_HEADER_SIZE, o) != OEP_HEADER_SIZE ||
        fwrite(data, 1, (size_t)sz, o) != (size_t)sz) {
        fprintf(stderr, "oep-pack: escrita falhou\n");
        fclose(o); free(data); return 1;
    }
    fclose(o);
    free(data);
    printf("pack: %s (%s, ELF %ld bytes) -> %s\n", name, category, sz, out);
    return 0;
}

/* ---- info (+ verificação de hash) ---- */
static int do_info(const char *path) {
    FILE *f = fopen(path, "rb");
    struct oep_header h;
    uint8_t *elf, digest[32];
    char sha[65];
    int i, ok;
    if (!f) { fprintf(stderr, "oep-info: %s: %s\n", path, strerror(errno)); return 1; }
    if (read_header(f, &h) != 0) {
        fprintf(stderr, "oep-info: %s: nao e .oep valido\n", path);
        fclose(f);
        return 1;
    }
    elf = malloc(h.elf_size);
    if (!elf) { fclose(f); return 1; }
    if (fseek(f, (long)h.elf_offset, SEEK_SET) != 0 ||
        fread(elf, 1, h.elf_size, f) != h.elf_size) {
        fprintf(stderr, "oep-info: ELF truncado\n");
        free(elf); fclose(f); return 1;
    }
    sha256(elf, h.elf_size, digest);
    ok = memcmp(digest, h.hash, 32) == 0;
    for (i = 0; i < 32; i++) sprintf(sha + i * 2, "%02x", h.hash[i]);
    printf("arquivo:   %s\n", path);
    printf("formato:   OEP v%u (header %u bytes)\n", h.version, OEP_HEADER_SIZE);
    printf("nome:      %s\n", h.name);
    printf("categoria: %s\n", h.category);
    printf("elf:       offset=%u tamanho=%u\n", h.elf_offset, h.elf_size);
    printf("sha256:    %s\n", sha);
    printf("hash:      %s\n", ok ? "OK" : "MISMATCH");
    free(elf);
    fclose(f);
    return ok ? 0 : 2;
}

/* ---- unpack ---- */
static int do_unpack(const char *path, const char *dir) {
    FILE *f = fopen(path, "rb"), *o;
    struct oep_header h;
    char out[1024];
    uint8_t *elf;
    if (!f) { fprintf(stderr, "oep-unpack: %s\n", strerror(errno)); return 1; }
    if (read_header(f, &h) != 0) {
        fprintf(stderr, "oep-unpack: nao e .oep valido\n");
        fclose(f); return 1;
    }
    mkdir(dir, 0755);
    snprintf(out, sizeof(out), "%s/%s.elf", dir, h.name);
    elf = malloc(h.elf_size);
    if (!elf) { fclose(f); return 1; }
    if (fseek(f, (long)h.elf_offset, SEEK_SET) != 0 ||
        fread(elf, 1, h.elf_size, f) != h.elf_size) {
        fprintf(stderr, "oep-unpack: ELF truncado\n");
        free(elf); fclose(f); return 1;
    }
    o = fopen(out, "wb");
    if (!o || fwrite(elf, 1, h.elf_size, o) != h.elf_size) {
        fprintf(stderr, "oep-unpack: %s: %s\n", out, strerror(errno));
        if (o) fclose(o);
        free(elf); fclose(f); return 1;
    }
    fclose(o);
    chmod(out, 0755);
    printf("unpack: %s -> %s (%u bytes)\n", path, out, h.elf_size);
    free(elf);
    fclose(f);
    return 0;
}

/* ---- run: extrai pro temp e execve ---- */
static int do_run(const char *path, int ac, char **av) {
    FILE *f = fopen(path, "rb");
    struct oep_header h;
    char cmd[1024];
    void *pbuf;
    if (!f) { fprintf(stderr, "oep-run: %s\n", strerror(errno)); return 1; }
    if (read_header(f, &h) != 0) {
        fprintf(stderr, "oep-run: nao e .oep valido\n");
        fclose(f); return 1;
    }
    snprintf(cmd, sizeof(cmd), "/tmp/%s.elf", h.name);
    pbuf = malloc(h.elf_size);
    if (!pbuf || fseek(f, (long)h.elf_offset, SEEK_SET) != 0 ||
        fread(pbuf, 1, h.elf_size, f) != h.elf_size) {
        fprintf(stderr, "oep-run: ELF truncado\n");
        free(pbuf); fclose(f); return 1;
    }
    fclose(f);
    {
        FILE *    o = fopen(cmd, "wb");
        if (!o || fwrite(pbuf, 1, h.elf_size, o) != h.elf_size) {
            fprintf(stderr, "oep-run: %s: %s\n", cmd, strerror(errno));
            if (o) fclose(o);
            free(pbuf); return 1;
        }
        fclose(o);
    }
    chmod(cmd, 0755);
    free(pbuf);
    {
        char **args = malloc(((size_t)ac + 2) * sizeof(char *));
        pid_t p; int st;
        if (!args) return 1;
        args[0] = cmd;
        memcpy(args + 1, av, (size_t)ac * sizeof(char *));
        args[ac + 1] = NULL;
        p = fork();
        if (p == 0) { execv(cmd, args); _exit(127); }
        while (waitpid(p, &st, 0) < 0) ;
        free(args);
        unlink(cmd);
        if (WIFEXITED(st)) return WEXITSTATUS(st);
        return 1;
    }
}

int main(int argc, char **argv) {
    const char *me = base(argv[0]);
    if (strstr(me, "info"))
        return argc == 2 ? do_info(argv[1]) : (fprintf(stderr, "uso: oep-info APP.oep\n"), 1);
    if (strstr(me, "unpack"))
        return argc == 3 ? do_unpack(argv[1], argv[2]) : (fprintf(stderr, "uso: oep-unpack APP.oep DIR/\n"), 1);
    if (strstr(me, "run"))
        return argc >= 2 ? do_run(argv[1], argc - 2, argv + 2) : (fprintf(stderr, "uso: oep-run APP.oep [args...]\n"), 1);
    /* pack (default): oep-pack <elf> <name> <category> -o out.oep */
    {
        const char *elf = NULL, *name = NULL, *category = "tools", *out = NULL;
        int i;
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out = argv[++i];
            else if (!elf) elf = argv[i];
            else if (!name) name = argv[i];
            else if (strcmp(category, "tools") == 0) category = argv[i];
        }
        if (!elf || !name || !out) {
            fprintf(stderr, "uso: oep-pack APP.elf NOME CATEGORIA -o APP.oep\n");
            return 1;
        }
        return do_pack(elf, name, category, out);
    }
}
