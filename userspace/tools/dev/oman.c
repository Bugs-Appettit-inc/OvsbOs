/* oman — man minimo: lista e exibe docs/ (texto paginado simples).
 * Uso: oman            (lista topicos)
 *      oman TOPICO     (exibe docs/TOPICO.md ou TOPICO)
 *      oman -k TERMO   (busca termo nos docs)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static const char *docdir(void) {
    const char *e = getenv("OMAN_PATH");
    return e ? e : "docs";
}

static void list(void) {
    DIR *d = opendir(docdir());
    struct dirent *e;
    if (!d) { printf("oman: sem %s/\n", docdir()); return; }
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        printf("%s\n", e->d_name);
    }
    closedir(d);
}

static void show(const char *topic) {
    char path[512];
    FILE *f;
    char line[512];
    int n = 0;
    snprintf(path, sizeof(path), "%s/%s", docdir(), topic);
    f = fopen(path, "r");
    if (!f) {
        snprintf(path, sizeof(path), "%s/%s.md", docdir(), topic);
        f = fopen(path, "r");
    }
    if (!f) { printf("oman: sem topico '%s'\n", topic); return; }
    while (fgets(line, sizeof(line), f)) {
        fputs(line, stdout);
        if (++n % 22 == 0) {
            printf("-- [Enter continua, q sai] --\n");
            fflush(stdout);
            {
                int c = getchar();
                if (c == 'q' || c == EOF) break;
            }
        }
    }
    fclose(f);
}

static void search(const char *term) {
    DIR *d = opendir(docdir());
    struct dirent *e;
    if (!d) { printf("oman: sem %s/\n", docdir()); return; }
    while ((e = readdir(d)) != NULL) {
        char path[512];
        FILE *f;
        char line[512];
        int ln = 0;
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", docdir(), e->d_name);
        f = fopen(path, "r");
        if (!f) continue;
        while (fgets(line, sizeof(line), f)) {
            ln++;
            if (strstr(line, term))
                printf("%s:%d: %s", e->d_name, ln, line);
        }
        fclose(f);
    }
    closedir(d);
}

int main(int argc, char **argv) {
    if (argc == 1) { list(); return 0; }
    if (strcmp(argv[1], "-k") == 0 && argc == 3) {
        search(argv[2]);
        return 0;
    }
    if (argc == 2) { show(argv[1]); return 0; }
    printf("uso: oman [TOPICO | -k TERMO]\n");
    return 1;
}
