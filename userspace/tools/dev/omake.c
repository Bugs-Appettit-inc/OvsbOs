/* omake — make minimo: le Makefile simples (alvo: deps + comandos TAB).
 * Uso: omake [alvo]  (default: primeiro alvo). Rebuild se alvo mais velho
 * que alguma dep (por mtime) ou se alvo nao existe. Executa via /bin/sh.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAXT 64
#define MAXD 32

typedef struct {
    char target[128];
    char deps[MAXD][128];
    int ndeps;
    char cmds[16][256];
    int ncmds;
} rule_t;

static long mtime(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return -1;
    return (long)st.st_mtime;
}

int main(int argc, char **argv) {
    FILE *f = fopen("Makefile", "r");
    char line[512];
    rule_t rules[MAXT];
    int nr = 0, i, want = 0;
    const char *goal = NULL;
    if (!f) { printf("omake: sem Makefile\n"); return 1; }
    if (argc > 1) goal = argv[1];
    while (fgets(line, sizeof(line), f)) {
        char *c;
        if (line[0] == '\t' || line[0] == ' ') {
            if (nr == 0 || rules[nr - 1].ncmds >= 16) continue;
            c = line;
            while (*c == '\t' || *c == ' ') c++;
            c[strcspn(c, "\n")] = 0;
            memcpy(rules[nr - 1].cmds[rules[nr - 1].ncmds++], c, 255);
            rules[nr - 1].cmds[rules[nr - 1].ncmds - 1][255] = 0;
            continue;
        }
        if (line[0] == '#' || line[0] == '\n') continue;
        c = strchr(line, ':');
        if (!c || nr >= MAXT) continue;
        *c = 0;
        memcpy(rules[nr].target, line, 127);
        rules[nr].target[127] = 0;
        rules[nr].ndeps = 0;
        rules[nr].ncmds = 0;
        {
            char *d = strtok(c + 1, " \t\n");
            while (d && rules[nr].ndeps < MAXD) {
                memcpy(rules[nr].deps[rules[nr].ndeps++], d, 127);
                rules[nr].deps[rules[nr].ndeps - 1][127] = 0;
                d = strtok(NULL, " \t\n");
            }
        }
        nr++;
    }
    fclose(f);
    if (nr == 0) { printf("omake: nenhuma regra\n"); return 1; }
    if (goal)
        for (i = 0; i < nr; i++)
            if (strcmp(rules[i].target, goal) == 0) want = i;
    for (i = 0; i < nr && !goal; i++)
        if (strcmp(rules[i].target, ".PHONY") != 0) {
            want = i;
            break;
        }
    {
        long tm = mtime(rules[want].target);
        int dirty = (tm < 0);
        int k;
        for (k = 0; k < rules[want].ndeps && !dirty; k++)
            if (mtime(rules[want].deps[k]) > tm) dirty = 1;
        if (!dirty) {
            printf("omake: '%s' atualizado\n", rules[want].target);
            return 0;
        }
        for (k = 0; k < rules[want].ncmds; k++) {
            printf("+ %s\n", rules[want].cmds[k]);
            if (system(rules[want].cmds[k]) != 0) {
                printf("omake: comando falhou\n");
                return 1;
            }
        }
    }
    return 0;
}
