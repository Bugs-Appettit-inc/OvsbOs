/* fm — gerenciador de arquivos TUI (+ --gui no framebuffer OvsbOS).
 *
 * Lista com getdents64 raw (Linux 217; OvsbOS traduz). Comandos:
 *   Enter abre dir | Backspace volta | o abre arquivo (editor/paint por ext)
 *   n nova pasta | r renomear | d deletar (confirma) | y yank | p colar copia
 *   v colar move | q sai. Na TUI digite a letra (+ Enter); setas movem sel.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include "../ovsb_fb.h"

#ifndef SYS_getdents64
#define SYS_getdents64 217
#endif

#define MAXE 1024
#define MAXP 1024

typedef struct {
    char name[256];
    int is_dir;
    long size;
} fment_t;

static fment_t g_e[MAXE];
static int g_n = 0, g_sel = 0;
static char g_cwd[MAXP] = "/";
static char g_yank[MAXP] = "";
static char g_msg[2048] = "";

struct dent64 {
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};

static int cmpent(const void *a, const void *b) {
    const fment_t *x = a, *y = b;
    if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;
    return strcmp(x->name, y->name);
}

static int fm_list(const char *dir) {
    static char dbuf[16384];
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    long n, off;
    g_n = 0;
    g_sel = 0;
    if (fd < 0) return -1;
    for (;;) {
        n = syscall(SYS_getdents64, fd, dbuf, sizeof(dbuf));
        if (n <= 0) break;
        off = 0;
        while (off < n && g_n < MAXE) {
            struct dent64 *e = (struct dent64 *)(dbuf + off);
            int nl;
            char full[MAXP];
            struct stat st;
            if (e->d_reclen < 19) break;
            nl = 0;
            while (nl + 19 < e->d_reclen && nl < 255 && e->d_name[nl])
                nl++;
            if (e->d_name[0] == '.' ||
                (nl == 1 && e->d_name[0] == '.')) {
                if (!(nl == 2 && e->d_name[1] == '.')) {
                    /* pula . e mostra .. como [<..>]? mostra sim */
                }
            }
            if (nl <= 0 || nl >= 255) { off += e->d_reclen; continue; }
            memcpy(g_e[g_n].name, e->d_name, (size_t)nl);
            g_e[g_n].name[nl] = 0;
            snprintf(full, sizeof(full), "%s/%s", dir, g_e[g_n].name);
            g_e[g_n].is_dir = (e->d_type == 4);
            g_e[g_n].size = -1;
            if (stat(full, &st) == 0) {
                g_e[g_n].is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
                g_e[g_n].size = (long)st.st_size;
            }
            g_n++;
            off += e->d_reclen;
        }
        if (g_n >= MAXE) break;
    }
    close(fd);
    qsort(g_e, (size_t)g_n, sizeof(fment_t), cmpent);
    return 0;
}

static void fm_up(void) {
    size_t l = strlen(g_cwd);
    if (l <= 1) return;
    while (l > 1 && g_cwd[l - 1] == '/') g_cwd[--l] = 0;
    while (l > 0 && g_cwd[l - 1] != '/') g_cwd[--l] = 0;
    if (l == 0) strcpy(g_cwd, "/");
    else if (l > 1) g_cwd[l - 1] = 0;
}

static int fm_copyfile(const char *s, const char *d) {
    int fi = open(s, O_RDONLY), fo;
    char b[8192];
    ssize_t r;
    if (fi < 0) return -1;
    fo = open(d, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fo < 0) { close(fi); return -1; }
    while ((r = read(fi, b, sizeof(b))) > 0)
        if (write(fo, b, (size_t)r) != r) {
            close(fi); close(fo); return -1;
        }
    close(fi);
    close(fo);
    return r < 0 ? -1 : 0;
}

static int fm_open_ext(void) {
    const char *nm;
    const char *dot;
    char cmd[MAXP + 64];
    int rc;
    if (g_n == 0) return -1;
    nm = g_e[g_sel].name;
    if (g_e[g_sel].is_dir) {
        size_t l = strlen(g_cwd);
        if (l + strlen(nm) + 2 >= sizeof(g_cwd)) return -1;
        if (strcmp(g_cwd, "/") != 0) strcat(g_cwd, "/");
        strcat(g_cwd, nm);
        return fm_list(g_cwd);
    }
    dot = strrchr(nm, '.');
    if (dot && (strcmp(dot, ".ppm") == 0 || strcmp(dot, ".PPM") == 0 ||
                strcmp(dot, ".bmp") == 0 || strcmp(dot, ".BMP") == 0))
        snprintf(cmd, sizeof(cmd), "paint --cli load '%s/%s'", g_cwd, nm);
    else
        snprintf(cmd, sizeof(cmd), "editor '%s/%s'", g_cwd, nm);
    rc = system(cmd);
    if (rc != 0)
        snprintf(g_msg, sizeof(g_msg),
                 "nao abri '%s' (editor/paint no PATH?)", nm);
    return 0;
}

static void fm_status(char *out, size_t n) {
    struct statvfs v;
    if (statvfs(g_cwd, &v) == 0) {
        unsigned long long freeb =
            (unsigned long long)v.f_bavail * v.f_frsize;
        snprintf(out, n, "%s  |  %d itens  |  livre: %llu KB", g_cwd, g_n,
                 freeb / 1024);
    } else {
        snprintf(out, n, "%s  |  %d itens", g_cwd, g_n);
    }
}

static void tui_draw(void) {
    char st[1152];
    int i, rows = 22;
    printf("\033[H\033[2J");
    fm_status(st, sizeof(st));
    printf("%s\n", st);
    printf("------------------------------------------------------------\n");
    for (i = 0; i < rows && i < g_n; i++) {
        char sz[16];
        if (g_e[i].is_dir)
            strcpy(sz, "<DIR>");
        else if (g_e[i].size >= 0)
            snprintf(sz, sizeof(sz), "%ld", g_e[i].size);
        else
            strcpy(sz, "?");
        printf("%c %-40.40s %10s\n", i == g_sel ? '>' : ' ',
               g_e[i].name, sz);
    }
    printf("------------------------------------------------------------\n");
    if (g_msg[0]) {
        printf("%s\n", g_msg);
        g_msg[0] = 0;
    }
    if (g_yank[0]) printf("[yank: %s]\n", g_yank);
    printf("Enter abre  Bksp volta  n/r/d/y/p/v/o/q  (+Enter)\n> ");
    fflush(stdout);
}

static void tui_prompt(const char *label, char *out, size_t n) {
    size_t m = 0;
    int c;
    printf("%s", label);
    fflush(stdout);
    if (!fgets(out, (int)n, stdin)) { out[0] = 0; return; }
    m = strlen(out);
    while (m > 0 && (out[m - 1] == '\n' || out[m - 1] == '\r'))
        out[--m] = 0;
    (void)c;
}

static void tui_run(void) {
    char line[512], arg[512], dst[MAXP];
    if (fm_list(g_cwd) != 0) {
        printf("nao abri %s: %s\n", g_cwd, strerror(errno));
        return;
    }
    for (;;) {
        tui_draw();
        if (!fgets(line, sizeof(line), stdin)) break;
        if (line[0] == 'q') break;
        else if (line[0] == '\n' || line[0] == '\r') fm_open_ext();
        else if (line[0] == '\177' || line[0] == '\b') {
            fm_up();
            fm_list(g_cwd);
        } else if (line[0] == 'j') {
            if (g_sel + 1 < g_n) g_sel++;
        } else if (line[0] == 'k') {
            if (g_sel > 0) g_sel--;
        } else if (line[0] == 'n') {
            tui_prompt("nova pasta: ", arg, sizeof(arg));
            if (arg[0]) {
                snprintf(dst, sizeof(dst), "%s/%s", g_cwd, arg);
                if (mkdir(dst, 0755) != 0)
                    snprintf(g_msg, sizeof(g_msg), "mkdir: %s",
                             strerror(errno));
                fm_list(g_cwd);
            }
        } else if (line[0] == 'r') {
            if (g_n > 0) {
                tui_prompt("renomear para: ", arg, sizeof(arg));
                if (arg[0]) {
                    char src[MAXP];
                    snprintf(src, sizeof(src), "%s/%s", g_cwd,
                             g_e[g_sel].name);
                    snprintf(dst, sizeof(dst), "%s/%s", g_cwd, arg);
                    if (rename(src, dst) != 0)
                        snprintf(g_msg, sizeof(g_msg), "rename: %s",
                                 strerror(errno));
                    fm_list(g_cwd);
                }
            }
        } else if (line[0] == 'd') {
            if (g_n > 0) {
                char src[MAXP];
                snprintf(src, sizeof(src), "%s/%s", g_cwd,
                         g_e[g_sel].name);
                tui_prompt("confirmar apagar (s/N): ", arg, sizeof(arg));
                if (arg[0] == 's' || arg[0] == 'S') {
                    int r = g_e[g_sel].is_dir ? rmdir(src) : unlink(src);
                    if (r != 0)
                        snprintf(g_msg, sizeof(g_msg), "apagar: %s",
                                 strerror(errno));
                    fm_list(g_cwd);
                }
            }
        } else if (line[0] == 'y') {
            if (g_n > 0)
                snprintf(g_yank, sizeof(g_yank), "%s/%s", g_cwd,
                         g_e[g_sel].name);
        } else if (line[0] == 'p' || line[0] == 'v') {
            if (g_yank[0]) {
                const char *base = strrchr(g_yank, '/');
                base = base ? base + 1 : g_yank;
                snprintf(dst, sizeof(dst), "%s/%s", g_cwd, base);
                if (line[0] == 'p') {
                    if (fm_copyfile(g_yank, dst) != 0)
                        snprintf(g_msg, sizeof(g_msg), "copiar: %s",
                                 strerror(errno));
                } else {
                    if (rename(g_yank, dst) != 0)
                        snprintf(g_msg, sizeof(g_msg), "mover: %s",
                                 strerror(errno));
                    else
                        g_yank[0] = 0;
                }
                fm_list(g_cwd);
            }
        } else if (line[0] == 'o') {
            fm_open_ext();
            fm_list(g_cwd);
        }
    }
}

static void gui_run(void) {
    app_fb_t fb;
    int quit = 0, top = 0;
    unsigned long cols, rows, i;
    if (!app_fb_init(&fb)) {
        printf("sem framebuffer neste ambiente (rode no OvsbOS com OVSBOS=1)\n");
        return;
    }
    if (fm_list(g_cwd) != 0) return;
    cols = fb.w / 8;
    rows = fb.h / 8;
    while (!quit) {
        int k = app_kbd_get();
        if (k == APP_KEY_UP && g_sel > 0) g_sel--;
        else if (k == APP_KEY_DOWN && g_sel + 1 < g_n) g_sel++;
        else if (k == APP_KEY_ENTER) { fm_open_ext(); }
        else if (k == APP_KEY_BS) { fm_up(); fm_list(g_cwd); }
        else if (k == 'q' || k == APP_KEY_ESC) quit = 1;
        else if (k == 'n') {
            char np[MAXP];
            snprintf(np, sizeof(np), "%s/nova-pasta", g_cwd);
            mkdir(np, 0755);
            fm_list(g_cwd);
        }
        if (g_sel < top) top = g_sel;
        if (g_sel >= top + (int)rows - 3) top = g_sel - (int)rows + 4;
        app_rect(&fb, 0, 0, fb.w, fb.h, 0xFF101418);
        app_text(&fb, 0, 0, g_cwd, 0xFFFFFF00, 0xFF101418);
        for (i = 0; i + 3 < rows && top + (int)i < g_n; i++) {
            char ln[256];
            snprintf(ln, sizeof(ln), "%c %s%s",
                     top + (int)i == g_sel ? '>' : ' ',
                     g_e[top + i].name,
                     g_e[top + i].is_dir ? "/" : "");
            ln[cols - 1] = 0;
            app_text(&fb, 0, (i + 1) * 8, ln,
                     top + (int)i == g_sel ? 0xFF000000 : 0xFFD0D0D0,
                     top + (int)i == g_sel ? 0xFF60A0FF : 0xFF101418);
        }
        app_text(&fb, 0, (rows - 1) * 8,
                 "Enter abre Bksp volta n pasta q sai", 0xFF808080,
                 0xFF101418);
        app_fb_flush(&fb);
    }
}

int main(int argc, char **argv) {
    int i;
    const char *home = getenv("HOME");
    if (home && home[0]) {
        strncpy(g_cwd, home, sizeof(g_cwd) - 1);
        g_cwd[sizeof(g_cwd) - 1] = 0;
    } else {
        if (!getcwd(g_cwd, sizeof(g_cwd))) strcpy(g_cwd, "/");
    }
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gui") == 0) { gui_run(); return 0; }
        if (argv[i][0] != '-') {
            strncpy(g_cwd, argv[i], sizeof(g_cwd) - 1);
            g_cwd[sizeof(g_cwd) - 1] = 0;
        }
    }
    tui_run();
    return 0;
}
