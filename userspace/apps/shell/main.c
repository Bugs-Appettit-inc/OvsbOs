/* shell — shell simples p/ OvsbOS (TUI; --gui abre console no fb).
 *
 * builtins: ls cd pwd cat echo mkdir rm cp mv clear exit help
 * historico: digitacao por linha (setas cima/baixo se o terminal mandar ANSI;
 *   no modo canonico use `!-n`... simples: historico via comando `hist`).
 * pipe simples (cmd1 | cmd2) e redirecionamento (> >> <) via fork/exec.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include "../ovsb_fb.h"

#ifndef SYS_getdents64
#define SYS_getdents64 217
#endif

#define MAXH 64
#define MAXL 1024

static char g_hist[MAXH][MAXL];
static int g_hn = 0;
static int g_status = 0;

static void sh_ls(const char *dir) {
    static char dbuf[16384];
    int fd = open(dir ? dir : ".", O_RDONLY | O_DIRECTORY);
    long n, off;
    if (fd < 0) { printf("ls: %s\n", strerror(errno)); return; }
    for (;;) {
        n = syscall(SYS_getdents64, fd, dbuf, sizeof(dbuf));
        if (n <= 0) break;
        off = 0;
        while (off < n) {
            long ino = *(long *)(dbuf + off);
            unsigned short rl = *(unsigned short *)(dbuf + off + 16);
            unsigned char tp = *(unsigned char *)(dbuf + off + 18);
            char *nm = dbuf + off + 19;
            int nl = 0;
            (void)ino;
            if (rl < 19) break;
            while (nl + 19 < rl && nl < 255 && nm[nl]) nl++;
            if (nl > 0 && nm[0] != '.')
                printf("%.*s%s\n", nl, nm, tp == 4 ? "/" : "");
            off += rl;
        }
    }
    close(fd);
}

static int copyfile(const char *s, const char *d) {
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

/* executa argv[] com redirecionamentos já resolvidos; retorna status. */
static int run(char **av, const char *in, const char *out, int app) {
    pid_t p;
    int st;
    fflush(stdout);
    fflush(stderr);
    p = fork();
    if (p < 0) { printf("fork: %s\n", strerror(errno)); return 1; }
    if (p == 0) {
        int fd;
        if (in) {
            fd = open(in, O_RDONLY);
            if (fd < 0) _exit(127);
            dup2(fd, 0);
            close(fd);
        }
        if (out) {
            fd = open(out, O_WRONLY | O_CREAT | (app ? O_APPEND : O_TRUNC),
                      0644);
            if (fd < 0) _exit(127);
            dup2(fd, 1);
            close(fd);
        }
        execvp(av[0], av);
        fprintf(stderr, "%s: comando nao encontrado\n", av[0]);
        _exit(127);
    }
    while (waitpid(p, &st, 0) < 0 && errno == EINTR)
        ;
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

/* tokeniza respeitando aspas simples/duplas; retorna argc. */
static int tokenize(char *line, char **av, int max) {
    int ac = 0;
    while (*line && ac + 1 < max) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line == 0) break;
        if (*line == '|') { av[ac++] = (char *)"|"; line++; continue; }
        if (*line == '<') { av[ac++] = (char *)"<"; line++; continue; }
        if (*line == '>') {
            if (line[1] == '>') {
                av[ac++] = (char *)">>";
                line += 2;
            } else {
                av[ac++] = (char *)">";
                line++;
            }
            continue;
        }
        {
            char *w = line, *d = line, q = 0, delim = 0;
            while (*line) {
                if (q) {
                    if (*line == q) { q = 0; line++; continue; }
                    *d++ = *line++;
                    continue;
                }
                if (*line == '\'' || *line == '"') {
                    q = *line;
                    line++;
                    continue;
                }
                if (*line == ' ' || *line == '\t' || *line == '|' ||
                    *line == '<' || *line == '>') {
                    delim = *line;
                    break;
                }
                *d++ = *line++;
            }
            if (*line == 0) delim = 0;
            *d = 0;
            av[ac++] = w;
            if (delim == 0) break;
            if (d == line) {
                /* Sem aspas o NUL acima clobberou o delimitador.
                 * Espaco/TAB: irrelevante. Operador: reemite. */
                if (delim == ' ' || delim == '\t') {
                    line++;
                } else {
                    if (delim == '|') av[ac++] = (char *)"|";
                    else if (delim == '<') av[ac++] = (char *)"<";
                    else av[ac++] = (char *)">";
                    line++;
                    if (delim == '>' && *line == '>') {
                        av[ac - 1] = (char *)">>";
                        line++;
                    }
                }
            }
            /* Com aspas (d<line) o delimitador segue intacto. */
        }
    }
    av[ac] = NULL;
    return ac;
}

/* Interior: quebra a linha em comandos separados por | com redirs. */
static int sh_exec(char *line) {
    char *av[128];
    char *left[64], *right[64];
    int ac, i, j, k, haspipe = -1;
    const char *in = NULL, *out = NULL;
    int app = 0;
    ac = tokenize(line, av, 128);
    if (ac == 0) return 0;
    /* builtins sem fork */
    if (strcmp(av[0], "exit") == 0) exit(g_status);
    if (strcmp(av[0], "cd") == 0) {
        const char *d = ac > 1 ? av[1] : getenv("HOME");
        if (!d) d = "/";
        if (chdir(d) != 0) printf("cd: %s\n", strerror(errno));
        return 0;
    }
    if (strcmp(av[0], "hist") == 0) {
        for (i = 0; i < g_hn; i++) printf("%d  %s\n", i, g_hist[i]);
        return 0;
    }
    for (i = 0; i < ac; i++)
        if (strcmp(av[i], "|") == 0) { haspipe = i; break; }
    if (haspipe < 0) {
        /* redirs */
        j = 0;
        for (i = 0; i < ac; i++) {
            if (strcmp(av[i], "<") == 0 && i + 1 < ac) in = av[++i];
            else if (strcmp(av[i], ">") == 0 && i + 1 < ac) {
                out = av[++i]; app = 0;
            } else if (strcmp(av[i], ">>") == 0 && i + 1 < ac) {
                out = av[++i]; app = 1;
            } else {
                av[j++] = av[i];
            }
        }
        av[j] = NULL;
        if (j == 0) return 0;
        if (strcmp(av[0], "ls") == 0 && !in && !out) {
            sh_ls(j > 1 ? av[1] : ".");
            return 0;
        }
        if (strcmp(av[0], "pwd") == 0 && !in && !out) {
            char c[1024];
            printf("%s\n", getcwd(c, sizeof(c)) ? c : "?");
            return 0;
        }
        if (strcmp(av[0], "echo") == 0) {
            if (!in && !out) {
                for (i = 1; i < j; i++)
                    printf("%s%s", av[i], i + 1 < j ? " " : "\n");
                if (j == 1) printf("\n");
                return 0;
            }
            if (!in && out) {
                /* sem fork: nao depende de /bin/echo no guest */
                int fd = open(out,
                              O_WRONLY | O_CREAT |
                                  (app ? O_APPEND : O_TRUNC),
                              0644);
                int k2, bad = 0;
                if (fd < 0) {
                    printf("echo: %s: %s\n", out, strerror(errno));
                    return 1;
                }
                for (k2 = 1; k2 < j; k2++) {
                    size_t wl = strlen(av[k2]);
                    if ((wl && write(fd, av[k2], wl) != (ssize_t)wl) ||
                        (k2 + 1 < j && write(fd, " ", 1) != 1) ||
                        (k2 + 1 == j && write(fd, "\n", 1) != 1))
                        bad = 1;
                }
                if (j == 1 && write(fd, "\n", 1) != 1) bad = 1;
                close(fd);
                return bad;
            }
            /* echo < in : cai no run (externo) */
        }
        if (strcmp(av[0], "clear") == 0 && !in && !out) {
            printf("\033[H\033[2J");
            return 0;
        }
        if (strcmp(av[0], "mkdir") == 0 && j > 1 && !in && !out) {
            int ii, bad = 0;
            for (ii = 1; ii < j; ii++)
                if (mkdir(av[ii], 0755) != 0) {
                    printf("mkdir: %s: %s\n", av[ii], strerror(errno));
                    bad = 1;
                }
            return bad;
        }
        if (strcmp(av[0], "rm") == 0 && j > 1 && !in && !out) {
            int ii, bad = 0;
            for (ii = 1; ii < j; ii++) {
                int r = unlink(av[ii]);
                if (r != 0) r = rmdir(av[ii]);
                if (r != 0) {
                    printf("rm: %s: %s\n", av[ii], strerror(errno));
                    bad = 1;
                }
            }
            return bad;
        }
        if (strcmp(av[0], "mv") == 0 && j > 2 && !in && !out) {
            if (rename(av[1], av[2]) != 0)
                printf("mv: %s\n", strerror(errno));
            return 0;
        }
        if (strcmp(av[0], "cp") == 0 && j > 2 && !in && !out) {
            if (copyfile(av[1], av[2]) != 0)
                printf("cp: %s\n", strerror(errno));
            return 0;
        }
        if (strcmp(av[0], "cat") == 0 && !out) {
            if (j == 1 || in) {
                /* cat via run p/ honrar < */
                return run(av, in, out, app);
            }
            for (i = 1; i < j; i++) {
                int fd = open(av[i], O_RDONLY);
                char b[4096];
                ssize_t r;
                if (fd < 0) {
                    printf("cat: %s: %s\n", av[i], strerror(errno));
                    continue;
                }
                while ((r = read(fd, b, sizeof(b))) > 0)
                    if (write(1, b, (size_t)r) != r) break;
                close(fd);
            }
            return 0;
        }
        if (strcmp(av[0], "help") == 0 && !in && !out) {
            printf("ls cd pwd cat echo mkdir rm cp mv clear exit hist help\n"
                   "pipe: a | b   redir: > >> <\n");
            return 0;
        }
        return run(av, in, out, app);
    }
    /* pipe simples: left | right (sem redir aninhado por simplicidade) */
    for (i = 0, j = 0; i < haspipe; i++) left[j++] = av[i];
    left[j] = NULL;
    for (i = haspipe + 1, k = 0; i < ac; i++) right[k++] = av[i];
    right[k] = NULL;
    if (!left[0] || !right[0]) {
        printf("pipe incompleto\n");
        return 1;
    }
    {
        int fds[2];
        pid_t p1, p2;
        int st;
        if (pipe(fds) != 0) {
            printf("pipe: %s\n", strerror(errno));
            return 1;
        }
        p1 = fork();
        if (p1 == 0) {
            dup2(fds[1], 1);
            close(fds[0]);
            close(fds[1]);
            execvp(left[0], left);
            _exit(127);
        }
        p2 = fork();
        if (p2 == 0) {
            dup2(fds[0], 0);
            close(fds[0]);
            close(fds[1]);
            execvp(right[0], right);
            _exit(127);
        }
        close(fds[0]);
        close(fds[1]);
        while (waitpid(p1, &st, 0) < 0 && errno == EINTR)
            ;
        while (waitpid(p2, &st, 0) < 0 && errno == EINTR)
            ;
        return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
    }
}

static void gui_run(void) {
    /* Console de emergencia no framebuffer: eco + execucao. */
    app_fb_t fb;
    char line[MAXL];
    size_t n = 0;
    int c;
    unsigned long row = 0;
    if (!app_fb_init(&fb)) {
        printf("sem framebuffer neste ambiente (rode no OvsbOS com OVSBOS=1)\n");
        return;
    }
    app_rect(&fb, 0, 0, fb.w, fb.h, 0xFF101418);
    app_text(&fb, 0, 0, "ovsb-sh (fb) — digite e Enter; 'exit' sai", 0xFFFFFF00,
             0xFF101418);
    row = 2;
    line[0] = 0;
    for (;;) {
        int k = app_kbd_get();
        if (k == APP_KEY_NONE) continue;
        if (k == APP_KEY_ENTER || k == '\n') {
            line[n] = 0;
            if (row + 2 < fb.h / 8) row += 2;
            if (strcmp(line, "exit") == 0) break;
            g_status = sh_exec(line);
            n = 0;
            line[0] = 0;
        } else if (k == APP_KEY_BS) {
            if (n > 0) { n--; line[n] = 0; }
        } else if (k >= 32 && k < 127 && n + 1 < sizeof(line)) {
            line[n++] = (char)k;
            line[n] = 0;
        } else if (k == APP_KEY_ESC) {
            break;
        }
        (void)c;
        app_rect(&fb, 0, (fb.h - 16), fb.w, 16, 0xFF101418);
        {
            char pr[MAXL + 4];
            snprintf(pr, sizeof(pr), "$ %s", line);
            app_text(&fb, 0, fb.h - 8, pr, 0xFFD0D0D0, 0xFF101418);
        }
        app_fb_flush(&fb);
    }
}


int main(int argc, char **argv) {
    char line[MAXL];
    int i;
    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "--gui") == 0) { gui_run(); return 0; }
    printf("ovsb-sh — digite 'help'. Ctrl+D sai.\n");
    for (;;) {
        char cwd[1024];
        printf("%s$ ", getcwd(cwd, sizeof(cwd)) ? cwd : "?");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = 0;
        line[strcspn(line, "\r")] = 0;
        if (!line[0]) continue;
        if (g_hn < MAXH) {
            memcpy(g_hist[g_hn], line, MAXL - 1);
            g_hist[g_hn][MAXL - 1] = 0;
            g_hn++;
        }
        g_status = sh_exec(line);
    }
    return g_status;
}
