/* oep.c — empacotador/listador/extrator .oep/.odk + loader.
 * Formato v1: OEP\0 | ver u16=1 | flags u16 | mlen u32 | nsec u32 |
 *   manifesto JSON | secoes(tipo u32, tam u32, dados).
 * Secoes: 1=ELF, 2=asset(nome NUL+dados), 3=icone.
 * Uso: oep-pack app.elf assets/ -o app.oep | oep-list app.oep |
 *      oep-unpack app.oep dir/ | oep-run app.oep [args...]
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define OEP_MAGIC "OEP"
#define ODK_MAGIC "ODK"
#define SEC_ELF 1
#define SEC_ASSET 2
#define SEC_ICON 3

static void w16(FILE *f, unsigned v) {
    fputc((int)(v & 0xFF), f);
    fputc((int)((v >> 8) & 0xFF), f);
}

static void w32(FILE *f, unsigned long v) {
    fputc((int)(v & 0xFF), f);
    fputc((int)((v >> 8) & 0xFF), f);
    fputc((int)((v >> 16) & 0xFF), f);
    fputc((int)((v >> 24) & 0xFF), f);
}

static unsigned r32(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8) |
           ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

static int copy_file(FILE *dst, const char *path, unsigned long *sz) {
    FILE *f = fopen(path, "rb");
    char b[8192];
    size_t n;
    unsigned long t = 0;
    if (!f) return -1;
    while ((n = fread(b, 1, sizeof(b), f)) > 0) {
        if (fwrite(b, 1, n, dst) != n) { fclose(f); return -1; }
        t += (unsigned long)n;
    }
    fclose(f);
    if (sz) *sz = t;
    return ferror(dst) ? -1 : 0;
}

static const char *base(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* empacota: elf + arquivos de assets/ + manifesto gerado. */
static int do_pack(const char *elf, const char *assets, const char *out,
                   int odk) {
    FILE *o = fopen(out, "wb");
    char mani[1024];
    int mlen, nsec = odk ? 0 : 1, rc = 0;
    DIR *d;
    struct dirent *e;
    if (!o) { printf("abrir %s: %s\n", out, strerror(errno)); return 1; }
    if (assets) {
        d = opendir(assets);
        if (d) {
            while ((e = readdir(d)) != NULL) {
                if (e->d_name[0] == '.') continue;
                nsec++;
            }
            closedir(d);
        }
    }
    mlen = snprintf(mani, sizeof(mani),
                    "{\"name\":\"%s\",\"version\":1,\"entry\":\"app\"}",
                    base(elf));
    if (mlen < 0 || mlen >= (int)sizeof(mani)) {
        fclose(o);
        return 1;
    }
    fwrite(odk ? ODK_MAGIC : OEP_MAGIC, 1, 3, o);
    fputc(0, o);
    w16(o, 1);
    w16(o, 1);
    w32(o, (unsigned long)mlen);
    w32(o, (unsigned long)nsec);
    fwrite(mani, 1, (size_t)mlen, o);
    if (!odk) {
        FILE *f = fopen(elf, "rb");
        long sz;
        if (!f) {
            printf("abrir %s: %s\n", elf, strerror(errno));
            fclose(o);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        w32(o, SEC_ELF);
        w32(o, (unsigned long)sz);
        fclose(f);
        if (copy_file(o, elf, NULL) != 0) rc = 1;
    }
    if (assets && rc == 0) {
        d = opendir(assets);
        if (d) {
            while ((e = readdir(d)) != NULL) {
                char full[1024], nm[256];
                FILE *f;
                long sz;
                size_t nl;
                if (e->d_name[0] == '.') continue;
                snprintf(full, sizeof(full), "%s/%s", assets, e->d_name);
                f = fopen(full, "rb");
                if (!f) continue;
                fseek(f, 0, SEEK_END);
                sz = ftell(f);
                fclose(f);
                if (sz < 0) continue;
                nl = strlen(e->d_name) + 1;
                w32(o, SEC_ASSET);
                w32(o, (unsigned long)sz + (unsigned long)nl);
                fwrite(e->d_name, 1, nl, o);
                snprintf(nm, sizeof(nm), "%s", e->d_name);
                (void)nm;
                if (copy_file(o, full, NULL) != 0) { rc = 1; break; }
            }
            closedir(d);
        }
    }
    fclose(o);
    if (rc == 0) printf("pack: %s (%d secoes)\n", out, nsec);
    return rc;
}

static int read_hdr(FILE *f, char magic[4], int *nsec, char **mani) {
    unsigned char hd[16];
    unsigned mlen;
    if (fread(hd, 1, 16, f) != 16) return -1;
    memcpy(magic, hd, 4);
    if (hd[0] != 'O' || hd[3] != 0 ||
        !((hd[1] == 'E' && hd[2] == 'P') ||
          (hd[1] == 'D' && hd[2] == 'K')))
        return -1;
    if (hd[4] != 1 || hd[5] != 0) return -1;
    mlen = r32(hd + 8);
    *nsec = (int)r32(hd + 12);
    if (mlen > 65536 || *nsec < 0 || *nsec > 1024) return -1;
    *mani = malloc(mlen + 1);
    if (!*mani) return -1;
    if (mlen && fread(*mani, 1, mlen, f) != mlen) {
        free(*mani);
        return -1;
    }
    (*mani)[mlen] = 0;
    return 0;
}

static int do_list(const char *path) {
    FILE *f = fopen(path, "rb");
    char magic[4];
    int nsec, i;
    char *mani = NULL;
    if (!f) { printf("abrir: %s\n", strerror(errno)); return 1; }
    if (read_hdr(f, magic, &nsec, &mani) != 0) {
        printf("nao e .oep/.odk valido\n");
        fclose(f);
        return 1;
    }
    printf("tipo=%.3s manifesto=%s secoes=%d\n", magic, mani, nsec);
    for (i = 0; i < nsec; i++) {
        unsigned char sh[8];
        unsigned t, sz;
        long pos;
        if (fread(sh, 1, 8, f) != 8) break;
        t = r32(sh);
        sz = r32(sh + 4);
        pos = ftell(f);
        printf("  [%d] tipo=%u tamanho=%u", i, t, sz);
        if (t == SEC_ASSET && sz > 0) {
            char nm[256];
            size_t k = 0;
            int c;
            while (k + 1 < sizeof(nm) && k < sz && (c = fgetc(f)) > 0)
                nm[k++] = (char)c;
            nm[k] = 0;
            printf(" nome=%s", nm);
        }
        printf("\n");
        fseek(f, pos + (long)sz, SEEK_SET);
    }
    free(mani);
    fclose(f);
    return 0;
}

static int do_unpack(const char *path, const char *dir) {
    FILE *f = fopen(path, "rb");
    char magic[4], out[1024], nm[256];
    int nsec, i, n = 0;
    char *mani = NULL;
    mkdir(dir, 0755);
    if (!f) { printf("abrir: %s\n", strerror(errno)); return 1; }
    if (read_hdr(f, magic, &nsec, &mani) != 0) {
        printf("nao e .oep/.odk valido\n");
        fclose(f);
        return 1;
    }
    printf("manifesto: %s\n", mani);
    for (i = 0; i < nsec; i++) {
        unsigned char sh[8];
        unsigned t, sz, k = 0;
        int c;
        if (fread(sh, 1, 8, f) != 8) break;
        t = r32(sh);
        sz = r32(sh + 4);
        if (t == SEC_ELF)
            snprintf(nm, sizeof(nm), "app.bin");
        else if (t == SEC_ASSET) {
            while (k + 1 < sizeof(nm) && k < sz && (c = fgetc(f)) > 0)
                nm[k++] = (char)c;
            nm[k] = 0;
            sz -= k + (k < sz ? 1 : 0);
        } else {
            snprintf(nm, sizeof(nm), "sec%d.bin", i);
        }
        snprintf(out, sizeof(out), "%s/%s", dir, nm);
        {
            FILE *o = fopen(out, "wb");
            unsigned left = sz;
            char b[4096];
            if (!o) {
                printf("criar %s: %s\n", out, strerror(errno));
                break;
            }
            while (left > 0) {
                size_t want = left > sizeof(b) ? sizeof(b) : left;
                size_t got = fread(b, 1, want, f);
                if (got == 0) break;
                fwrite(b, 1, got, o);
                left -= (unsigned)got;
            }
            fclose(o);
            if (t == SEC_ELF) chmod(out, 0755);
            printf("  -> %s (%u bytes)\n", out, sz - left);
            n++;
        }
    }
    free(mani);
    fclose(f);
    return 0;
}

/* loader: extrai ELF p/ temp e execve. */
static int do_run(const char *path, int ac, char **av) {
    char tmp[] = "/tmp/oep-run-XXXXXX";
    int fd = mkstemp(tmp);
    FILE *f;
    char magic[4], *mani = NULL;
    int nsec, i;
    if (fd < 0) { printf("temp: %s\n", strerror(errno)); return 1; }
    close(fd);
    f = fopen(path, "rb");
    if (!f) { printf("abrir: %s\n", strerror(errno)); return 1; }
    if (read_hdr(f, magic, &nsec, &mani) != 0 || magic[1] != 'E') {
        printf("oep-run precisa de .oep com ELF\n");
        fclose(f);
        return 1;
    }
    free(mani);
    for (i = 0; i < nsec; i++) {
        unsigned char sh[8];
        unsigned t, sz;
        if (fread(sh, 1, 8, f) != 8) break;
        t = r32(sh);
        sz = r32(sh + 4);
        if (t == SEC_ELF) {
            FILE *o = fopen(tmp, "wb");
            unsigned left = sz;
            char b[4096];
            if (!o) break;
            while (left > 0) {
                size_t want = left > sizeof(b) ? sizeof(b) : left;
                size_t got = fread(b, 1, want, f);
                if (got == 0) break;
                fwrite(b, 1, got, o);
                left -= (unsigned)got;
            }
            fclose(o);
            chmod(tmp, 0755);
            break;
        }
        fseek(f, (long)sz, SEEK_CUR);
    }
    fclose(f);
    {
        char **args = malloc((size_t)(ac + 2) * sizeof(char *));
        pid_t p;
        int st;
        if (!args) return 1;
        args[0] = tmp;
        memcpy(args + 1, av, (size_t)ac * sizeof(char *));
        args[ac + 1] = NULL;
        p = fork();
        if (p == 0) {
            execv(tmp, args);
            _exit(127);
        }
        while (waitpid(p, &st, 0) < 0)
            ;
        unlink(tmp);
        free(args);
        if (WIFEXITED(st)) return WEXITSTATUS(st);
        return 1;
    }
}

int main(int argc, char **argv) {
    const char *me = base(argv[0]);
    if (strstr(me, "unpack")) {
        if (argc != 3) {
            printf("uso: oep-unpack APP.oep DIR/\n");
            return 1;
        }
        return do_unpack(argv[1], argv[2]);
    }
    if (strstr(me, "list")) {
        if (argc != 2) {
            printf("uso: oep-list APP.oep\n");
            return 1;
        }
        return do_list(argv[1]);
    }
    if (strstr(me, "run")) {
        if (argc < 2) {
            printf("uso: oep-run APP.oep [args...]\n");
            return 1;
        }
        return do_run(argv[1], argc - 2, argv + 2);
    }
    /* pack (default): oep-pack app.elf assets/ -o app.oep */
    {
        const char *elf = NULL, *assets = NULL, *out = NULL;
        int i, odk = (strstr(me, "odk") != NULL);
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
                out = argv[++i];
            else if (!elf)
                elf = argv[i];
            else if (!assets)
                assets = argv[i];
        }
        if (!elf || !out) {
            printf("uso: oep-pack APP.elf [ASSETS/] -o APP.oep\n");
            return 1;
        }
        return do_pack(elf, assets, out, odk);
    }
}
