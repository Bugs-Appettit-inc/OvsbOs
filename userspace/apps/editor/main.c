/* editor — editor de texto TUI (+ modo --gui via framebuffer OvsbOS).
 *
 * Uso: editor [arquivo] [--gui]
 * Teclas: setas, Home/End, PgUp/PgDn, Backspace/Del, Enter,
 *         Ctrl+S salva, Ctrl+F busca, Ctrl+Q sai.
 * UTF-8: bytes >=0x80 passam intactos (cursor anda por byte).
 * Binario: NUL e exibido como '~' na tela, preservado ao salvar.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../ovsb_fb.h"
#include "../../libs/image/image.h"

#define MAXL 512
#define MAXC 512

static char g_buf[MAXL][MAXC];
static int g_len[MAXL];
static int g_nlines = 1;
static int g_cx = 0, g_cy = 0, g_rowoff = 0, g_coloff = 0;
static int g_dirty = 0;
static char g_path[256] = "";
static char g_msg[512] = "";

/* ---------- modelo ---------- */
static void ed_insert(int ch) {
    if (g_cy < 0 || g_cy >= g_nlines) return;
    if (g_len[g_cy] >= MAXC - 1 || ch == 0) return;
    if (g_cx > g_len[g_cy]) g_cx = g_len[g_cy];
    memmove(&g_buf[g_cy][g_cx + 1], &g_buf[g_cy][g_cx],
            (size_t)(g_len[g_cy] - g_cx));
    g_buf[g_cy][g_cx] = (char)ch;
    g_len[g_cy]++;
    g_cx++;
    g_dirty = 1;
}

static void ed_enter(void) {
    int rest;
    if (g_nlines >= MAXL) return;
    if (g_cx > g_len[g_cy]) g_cx = g_len[g_cy];
    rest = g_len[g_cy] - g_cx;
    memmove(g_buf[g_nlines], g_buf[g_cy + 1],
            (size_t)(g_nlines - g_cy - 1) * MAXC);
    memmove(g_len + g_cy + 1, g_len + g_cy,
            (size_t)(g_nlines - g_cy - 1) * sizeof(int));
    memcpy(g_buf[g_cy + 1], &g_buf[g_cy][g_cx], (size_t)rest);
    g_len[g_cy + 1] = rest;
    g_len[g_cy] = g_cx;
    g_nlines++;
    g_cy++;
    g_cx = 0;
    g_dirty = 1;
}

static void ed_backspace(void) {
    int prev;
    if (g_cx > 0) {
        if (g_cx > g_len[g_cy]) g_cx = g_len[g_cy];
        memmove(&g_buf[g_cy][g_cx - 1], &g_buf[g_cy][g_cx],
                (size_t)(g_len[g_cy] - g_cx));
        g_len[g_cy]--;
        g_cx--;
        g_dirty = 1;
    } else if (g_cy > 0) {
        prev = g_len[g_cy - 1];
        if (prev + g_len[g_cy] < MAXC) {
            memcpy(&g_buf[g_cy - 1][prev], g_buf[g_cy],
                   (size_t)g_len[g_cy]);
            g_len[g_cy - 1] = prev + g_len[g_cy];
            memmove(g_buf[g_cy], g_buf[g_cy + 1],
                    (size_t)(g_nlines - g_cy - 1) * MAXC);
            memmove(g_len + g_cy, g_len + g_cy + 1,
                    (size_t)(g_nlines - g_cy - 1) * sizeof(int));
            g_nlines--;
            g_cy--;
            g_cx = prev;
            g_dirty = 1;
        }
    }
}

static void ed_del(void) {
    int rest;
    if (g_cx < g_len[g_cy]) {
        memmove(&g_buf[g_cy][g_cx], &g_buf[g_cy][g_cx + 1],
                (size_t)(g_len[g_cy] - g_cx - 1));
        g_len[g_cy]--;
        g_dirty = 1;
    } else if (g_cy + 1 < g_nlines) {
        rest = g_len[g_cy + 1];
        if (g_len[g_cy] + rest < MAXC) {
            memcpy(&g_buf[g_cy][g_len[g_cy]], g_buf[g_cy + 1],
                   (size_t)rest);
            g_len[g_cy] += rest;
            memmove(g_buf[g_cy + 1], g_buf[g_cy + 2],
                    (size_t)(g_nlines - g_cy - 2) * MAXC);
            memmove(g_len + g_cy + 1, g_len + g_cy + 2,
                    (size_t)(g_nlines - g_cy - 2) * sizeof(int));
            g_nlines--;
            g_dirty = 1;
        }
    }
}

static int ed_load(const char *path) {
    FILE *f = fopen(path, "rb");
    int c;
    int overflow = 0;
    g_nlines = 1;
    g_len[0] = 0;
    if (!f) return -1;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            if (g_nlines >= MAXL) { overflow = 1; break; }
            g_len[g_nlines] = 0;
            g_nlines++;
        } else {
            if (g_len[g_nlines - 1] < MAXC - 1)
                g_buf[g_nlines - 1][g_len[g_nlines - 1]++] = (char)c;
            else
                overflow = 1;
        }
    }
    fclose(f);
    g_dirty = 0;
    if (overflow)
        snprintf(g_msg, sizeof(g_msg),
                 "aviso: arquivo truncado (limite %dx%d)", MAXL, MAXC);
    return 0;
}

static int ed_save(const char *path) {
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return -1;
    for (i = 0; i < g_nlines; i++) {
        if (g_len[i] > 0 &&
            fwrite(g_buf[i], 1, (size_t)g_len[i], f) != (size_t)g_len[i]) {
            fclose(f);
            return -1;
        }
        if (i + 1 < g_nlines) fputc('\n', f);
    }
    fclose(f);
    g_dirty = 0;
    return 0;
}

/* busca literal a partir do cursor; retorna 1 se achou (move cursor). */
static int ed_find(const char *q) {
    size_t ql = strlen(q);
    int y, x, k, ok;
    if (ql == 0 || ql >= MAXC) return 0;
    for (y = g_cy; y < g_nlines; y++) {
        int x0 = (y == g_cy) ? g_cx + 1 : 0;
        for (x = x0; x + (int)ql <= g_len[y]; x++) {
            ok = 1;
            for (k = 0; k < (int)ql; k++)
                if (g_buf[y][x + k] != q[k]) { ok = 0; break; }
            if (ok) {
                g_cy = y;
                g_cx = x;
                return 1;
            }
        }
    }
    return 0;
}

/* ---------- terminal ---------- */
static void term_raw(int on) {
    /* Sem termios aqui de proposito: no guest OvsbOS o stdin do console ja
     * entrega caracteres; no host, rode dentro de um terminal (o driver
     * interpreta as sequencias ANSI em term_getkey). */
    (void)on;
}

/* Le uma tecla: retorna ASCII, 256+ especial (mesmo mapa do APP_KEY_*),
 * -1 em EOF/erro. Via stdin; entende ANSI (ESC [ A/B/C/D/H/F/5~/6~/3~). */
static int term_getkey(void) {
    int c = getchar();
    int c2, c3;
    if (c == EOF) return -1;
    if (c != 27) return c;
    c2 = getchar();
    if (c2 == EOF) return APP_KEY_ESC;
    if (c2 != '[' && c2 != 'O') return APP_KEY_ESC;
    c3 = getchar();
    if (c3 == EOF) return APP_KEY_ESC;
    switch (c3) {
    case 'A': return APP_KEY_UP;
    case 'B': return APP_KEY_DOWN;
    case 'C': return APP_KEY_RIGHT;
    case 'D': return APP_KEY_LEFT;
    case 'H': return APP_KEY_HOME;
    case 'F': return APP_KEY_END;
    case '3': if (getchar() == '~') return APP_KEY_DEL; return APP_KEY_ESC;
    case '5': if (getchar() == '~') return APP_KEY_PGUP; return APP_KEY_ESC;
    case '6': if (getchar() == '~') return APP_KEY_PGDN; return APP_KEY_ESC;
    default: return APP_KEY_ESC;
    }
}

static void term_clear(void) {
    printf("\033[H\033[2J");
}

static int g_hl = 0; /* 0=off 1=C 2=asm 3=sh */
static int g_lnum = 0;

static int is_wordch(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

/* imprime linha com highlight ANSI; retorna void. */
static void hl_print(const char *s, int n) {
    static const char *kw_c =
        "|int|char|long|short|void|static|const|unsigned|signed|struct|"
        "enum|typedef|return|if|else|for|while|do|switch|case|break|"
        "continue|sizeof|include|define|ifdef|ifndef|endif|";
    static const char *kw_asm =
        "|mov|add|sub|push|pop|call|ret|jmp|je|jne|jg|jl|cmp|test|xor|"
        "and|or|lea|nop|int|iretq|syscall|xchg|rdmsr|wrmsr|invlpg|sti|cli|";
    static const char *kw_sh =
        "|if|then|else|fi|for|while|do|done|case|esac|function|return|"
        "echo|exit|export|local|read|cd|ls|exec|trap|";
    const char *kw = g_hl == 1 ? kw_c : g_hl == 2 ? kw_asm : kw_sh;
    int i = 0;
    while (i < n) {
        char c = s[i];
        if (c == '/' && g_hl == 1 && i + 1 < n && s[i + 1] == '/') {
            printf("\033[32m");
            while (i < n) putchar(s[i++]);
            printf("\033[0m");
            break;
        }
        if (c == '#' && (g_hl == 2 || g_hl == 3)) {
            printf("\033[32m");
            while (i < n) putchar(s[i++]);
            printf("\033[0m");
            break;
        }
        if (c == '"' || c == '\'') {
            char q = c;
            printf("\033[33m%c", c);
            i++;
            while (i < n && s[i] != q) {
                if (s[i] == '\\' && i + 1 < n) {
                    putchar(s[i]);
                    i++;
                }
                putchar(s[i]);
                i++;
            }
            if (i < n) putchar(s[i++]);
            printf("\033[0m");
            continue;
        }
        if (is_wordch(c) && (i == 0 || !is_wordch(s[i - 1]))) {
            int j = i;
            char w[32];
            int k = 0;
            while (j < n && is_wordch(s[j]) && k < 30) w[k++] = s[j++];
            w[k] = 0;
            {
                char pat[34];
                snprintf(pat, sizeof(pat), "|%s|", w);
                if (strstr(kw, pat)) {
                    printf("\033[1;36m%s\033[0m", w);
                    i = j;
                    continue;
                }
            }
        }
        if (c >= '0' && c <= '9' && (i == 0 || !is_wordch(s[i - 1]))) {
            printf("\033[35m");
            while (i < n && (is_wordch(s[i]) || s[i] == 'x' || s[i] == '.'))
                putchar(s[i++]);
            printf("\033[0m");
            continue;
        }
        putchar(c == 0 ? '~' : c);
        i++;
    }
}

static void term_draw(int rows, int cols) {
    int y, x, li;
    (void)cols;
    term_clear();
    for (y = 0; y < rows - 2; y++) {
        li = g_rowoff + y;
        if (li < g_nlines) {
            int start = g_coloff;
            int n = g_len[li] - start;
            if (n < 0) n = 0;
            if (n > cols - 1) n = cols - 1;
            if (g_lnum) printf("\033[90m%4d \033[0m", li + 1);
            if (g_hl)
                hl_print(&g_buf[li][start], n);
            else
                for (x = 0; x < n; x++) {
                    char ch = g_buf[li][start + x];
                    putchar(ch == 0 ? '~' : ch);
                }
        } else {
            putchar('~');
        }
        putchar('\n');
    }
    printf("-- %s %s -- %d/%d --\n",
           g_path[0] ? g_path : "[sem nome]",
           g_dirty ? "(modificado)" : "(salvo)", g_cy + 1, g_nlines);
    if (g_msg[0]) {
        printf("%s\n", g_msg);
        g_msg[0] = 0;
    } else {
        printf("Ctrl+S salva  Ctrl+F busca  Ctrl+Q sai\n");
    }
    fflush(stdout);
}

static void term_prompt(const char *label, char *out, size_t outsz) {
    size_t n = 0;
    int c;
    printf("%s", label);
    fflush(stdout);
    while (n + 1 < outsz) {
        c = getchar();
        if (c == EOF || c == '\n' || c == '\r') break;
        if (c == 127 || c == 8) {
            if (n > 0) n--;
            continue;
        }
        out[n++] = (char)c;
    }
    out[n] = 0;
}

/* ---------- modo GUI (framebuffer) ---------- */
static void gui_run(void) {
    app_fb_t fb;
    int quit = 0;
    unsigned long cols, rows, vy, vx, li, x;
    if (!app_fb_init(&fb)) {
        printf("sem framebuffer neste ambiente (rode no OvsbOS com OVSBOS=1)\n");
        return;
    }
    cols = fb.w / 8;
    rows = fb.h / 8;
    while (!quit) {
        int k = app_kbd_get();
        if (k >= 32 && k < 127) {
            ed_insert(k);
        } else if (k == APP_KEY_ENTER) {
            ed_enter();
        } else if (k == APP_KEY_BS) {
            ed_backspace();
        } else if (k == APP_KEY_DEL) {
            ed_del();
        } else if (k == APP_KEY_UP) {
            if (g_cy > 0) g_cy--;
        } else if (k == APP_KEY_DOWN) {
            if (g_cy + 1 < g_nlines) g_cy++;
        } else if (k == APP_KEY_LEFT) {
            if (g_cx > 0) g_cx--;
        } else if (k == APP_KEY_RIGHT) {
            g_cx++;
        } else if (k == 19) { /* Ctrl+S */
            if (g_path[0]) {
                if (ed_save(g_path) == 0)
                    snprintf(g_msg, sizeof(g_msg), "salvo!");
                else
                    snprintf(g_msg, sizeof(g_msg), "erro ao salvar");
            }
        } else if (k == 17) { /* Ctrl+Q */
            quit = 1;
        }
        if (g_cx > g_len[g_cy]) g_cx = g_len[g_cy];
        if (g_cx < 0) g_cx = 0;
        if (g_cy < g_rowoff) g_rowoff = g_cy;
        if (g_cy >= g_rowoff + (int)rows - 2)
            g_rowoff = g_cy - (int)rows + 3;
        app_rect(&fb, 0, 0, fb.w, fb.h, 0xFF101418);
        for (vy = 0; vy + 2 < rows; vy++) {
            li = (unsigned long)(g_rowoff + (int)vy);
            if ((int)li >= g_nlines) break;
            for (x = 0; x < cols && (int)x < g_len[li]; x++) {
                char ch = g_buf[li][x];
                app_char(&fb, x * 8, vy * 8, ch == 0 ? '~' : ch,
                         0xFFD0D0D0, 0xFF101418);
            }
            if ((int)li == g_cy)
                app_rect(&fb, (unsigned long)g_cx * 8, vy * 8, 8, 8,
                         0xFF4060A0);
        }
        app_text(&fb, 0, (rows - 1) * 8, g_msg[0] ? g_msg : "Ctrl+S salva  Ctrl+Q sai",
                 0xFFFFFF00, 0xFF101418);
        app_fb_flush(&fb);
        (void)vx;
    }
}

/* ---------- main ---------- */
int main(int argc, char **argv) {
    int use_gui = 0;
    int i, k, rows = 24, cols = 80;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gui") == 0)
            use_gui = 1;
        else if (strcmp(argv[i], "--ln") == 0)
            g_lnum = 1;
        else if (strncmp(argv[i], "--hl=", 5) == 0) {
            if (strcmp(argv[i] + 5, "c") == 0) g_hl = 1;
            else if (strcmp(argv[i] + 5, "asm") == 0) g_hl = 2;
            else if (strcmp(argv[i] + 5, "sh") == 0) g_hl = 3;
            else {
                printf("hl: c|asm|sh\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--dump-img") == 0) {
            img_t *im;
            if (++i >= argc) {
                printf("uso: editor --dump-img ARQUIVO\n");
                return 1;
            }
            im = img_load(argv[i]);
            if (!im) {
                printf("nao e imagem (bmp/ppm/png): %s\n", argv[i]);
                return 1;
            }
            img_ascii(im, 72);
            img_free(im);
            return 0;
        } else if (argv[i][0] != '-') {
            strncpy(g_path, argv[i], sizeof(g_path) - 1);
            g_path[sizeof(g_path) - 1] = 0;
        }
    }
    if (g_path[0] && ed_load(g_path) != 0)
        snprintf(g_msg, sizeof(g_msg), "[novo arquivo]");
    if (use_gui) {
        gui_run();
        return 0;
    }
    term_raw(1);
    term_draw(rows, cols);
    for (;;) {
        k = term_getkey();
        if (k == -1) break;
        if (k == 17) break;              /* Ctrl+Q */
        else if (k == 19) {              /* Ctrl+S */
            if (!g_path[0])
                term_prompt("salvar como: ", g_path, sizeof(g_path));
            if (g_path[0]) {
                if (ed_save(g_path) == 0)
                    snprintf(g_msg, sizeof(g_msg), "salvo em %s", g_path);
                else
                    snprintf(g_msg, sizeof(g_msg), "ERRO ao salvar %s",
                             g_path);
            }
        } else if (k == 6) {             /* Ctrl+F */
            char q[128];
            term_prompt("buscar: ", q, sizeof(q));
            if (!ed_find(q))
                snprintf(g_msg, sizeof(g_msg), "nao achado: %s", q);
        } else if (k == 18) {            /* Ctrl+R: substituir */
            char q[128], r[128];
            int cnt = 0, y, x, i2, j2;
            size_t ql, rl;
            term_prompt("substituir: ", q, sizeof(q));
            term_prompt("por: ", r, sizeof(r));
            ql = strlen(q);
            rl = strlen(r);
            if (ql > 0 && ql < MAXC && rl < MAXC) {
                for (y = 0; y < g_nlines; y++) {
                    x = 0;
                    while (x + (int)ql <= g_len[y]) {
                        int ok = 1;
                        for (i2 = 0; i2 < (int)ql; i2++)
                            if (g_buf[y][x + i2] != q[i2]) {
                                ok = 0;
                                break;
                            }
                        if (!ok) { x++; continue; }
                        if (g_len[y] - (int)ql + (int)rl < MAXC) {
                            memmove(&g_buf[y][x + rl],
                                    &g_buf[y][x + ql],
                                    (size_t)(g_len[y] - x - (int)ql));
                            for (j2 = 0; j2 < (int)rl; j2++)
                                g_buf[y][x + j2] = r[j2];
                            g_len[y] += (int)rl - (int)ql;
                            x += (int)rl;
                            cnt++;
                            g_dirty = 1;
                        } else {
                            x++;
                        }
                    }
                }
                snprintf(g_msg, sizeof(g_msg), "%d substituicoes", cnt);
            }
        } else if (k == 14) {            /* Ctrl+N: numeros de linha */
            g_lnum = !g_lnum;
        } else if (k == APP_KEY_UP) {
            if (g_cy > 0) g_cy--;
        } else if (k == APP_KEY_DOWN) {
            if (g_cy + 1 < g_nlines) g_cy++;
        } else if (k == APP_KEY_LEFT) {
            if (g_cx > 0) g_cx--;
        } else if (k == APP_KEY_RIGHT) {
            g_cx++;
        } else if (k == APP_KEY_HOME) {
            g_cx = 0;
        } else if (k == APP_KEY_END) {
            g_cx = g_len[g_cy];
        } else if (k == APP_KEY_PGUP) {
            g_cy -= (rows - 3);
            if (g_cy < 0) g_cy = 0;
        } else if (k == APP_KEY_PGDN) {
            g_cy += (rows - 3);
            if (g_cy >= g_nlines) g_cy = g_nlines - 1;
        } else if (k == 127 || k == APP_KEY_BS) {
            ed_backspace();
        } else if (k == APP_KEY_DEL) {
            ed_del();
        } else if (k == '\n' || k == '\r' || k == APP_KEY_ENTER) {
            ed_enter();
        } else if ((k >= 32 && k < 127) || k >= 128) {
            ed_insert(k);
        }
        if (g_cx > g_len[g_cy]) g_cx = g_len[g_cy];
        if (g_cx < 0) g_cx = 0;
        if (g_cy < g_rowoff) g_rowoff = g_cy;
        if (g_cy >= g_rowoff + rows - 2) g_rowoff = g_cy - rows + 3;
        if (g_cx < g_coloff) g_coloff = g_cx;
        if (g_cx >= g_coloff + cols) g_coloff = g_cx - cols + 1;
        term_draw(rows, cols);
    }
    term_raw(0);
    term_clear();
    if (g_dirty)
        printf("[editor] alteracoes NAO salvas descartadas.\n");
    return 0;
}
