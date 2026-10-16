/* oasm — mini-assembler de brinquedo (demonstracao, nao gera ELF).
 * Subconjunto: mov reg,imm32 | add reg,imm32 | jmp rotulo | rotulo:
 * Regs: eax ecx edx ebx. Saida: binario flat (lista de opcodes x86-32).
 * Uso: oasm in.asm -o out.bin
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int regno(const char *s) {
    if (strcmp(s, "eax") == 0) return 0;
    if (strcmp(s, "ecx") == 0) return 1;
    if (strcmp(s, "edx") == 0) return 2;
    if (strcmp(s, "ebx") == 0) return 3;
    return -1;
}

static void trim(char *s) {
    char *e;
    while (*s == ' ' || *s == '\t') memmove(s, s + 1, strlen(s));
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' ||
                     e[-1] == '\r'))
        *--e = 0;
}

int main(int argc, char **argv) {
    FILE *fin, *fout;
    char line[256], *out = NULL;
    char labels[64][48];
    int labaddr[64], nlab = 0, addr = 0, ln = 0, fails = 0, i;
    if (argc != 4 || strcmp(argv[2], "-o") != 0) {
        printf("uso: oasm in.asm -o out.bin\n"
               "  mov reg,imm | add reg,imm | jmp rotulo | rotulo:\n");
        return 1;
    }
    fin = fopen(argv[1], "r");
    if (!fin) { printf("abrir %s\n", argv[1]); return 1; }
    out = argv[3];
    /* passe 1: rotulos */
    while (fgets(line, sizeof(line), fin)) {
        char *c;
        ln++;
        trim(line);
        if (!line[0] || line[0] == ';') continue;
        c = strchr(line, ':');
        if (c && c[1] == 0) {
            *c = 0;
            trim(line);
            if (nlab < 64) {
                memcpy(labels[nlab], line, 47);
                labels[nlab][47] = 0;
                labaddr[nlab] = addr;
                nlab++;
            }
            continue;
        }
        if (strncmp(line, "mov", 3) == 0 || strncmp(line, "add", 3) == 0)
            addr += 5;
        else if (strncmp(line, "jmp", 3) == 0)
            addr += 5;
        else {
            printf("linha %d: mnemonico? (%s)\n", ln, line);
            fails++;
        }
    }
    if (fails) { fclose(fin); return 1; }
    fout = fopen(out, "wb");
    if (!fout) { printf("criar %s\n", out); fclose(fin); return 1; }
    /* passe 2: emite */
    fseek(fin, 0, SEEK_SET);
    ln = 0;
    {
        int cur = 0;
        while (fgets(line, sizeof(line), fin)) {
        char *p, *a1, *a2;
        int r;
        long v;
        ln++;
        trim(line);
        if (!line[0] || line[0] == ';') continue;
        if (line[strlen(line) - 1] == ':') continue;
        for (p = line; *p && *p != ' ' && *p != '\t'; p++)
            ;
        if (*p) *p++ = 0;
        while (*p == ' ' || *p == '\t') p++;
        a1 = p;
        while (*p && *p != ',') p++;
        if (*p) *p++ = 0;
        a2 = p;
        trim(a1);
        trim(a2);
        if (strcmp(line, "jmp") == 0) {
            int t = -1;
            long rel;
            for (i = 0; i < nlab; i++)
                if (strcmp(labels[i], a1) == 0) t = labaddr[i];
            if (t < 0) {
                printf("linha %d: rotulo %s?\n", ln, a1);
                fails++;
                continue;
            }
            rel = (long)t - (long)(cur + 5);
            fputc(0xE9, fout);
            fputc((int)(rel & 0xFF), fout);
            fputc((int)((rel >> 8) & 0xFF), fout);
            fputc((int)((rel >> 16) & 0xFF), fout);
            fputc((int)((rel >> 24) & 0xFF), fout);
            cur += 5;
            continue;
        }
        r = regno(a1);
        v = strtol(a2, NULL, 0);
        if (r < 0) {
            printf("linha %d: reg %s?\n", ln, a1);
            fails++;
            continue;
        }
        if (strcmp(line, "mov") == 0) fputc(0xB8 + r, fout);
        else if (strcmp(line, "add") == 0) {
            /* add eax,imm32 = 05; demais: 81 /0 */
            if (r == 0) {
                fputc(0x05, fout);
            } else {
                fputc(0x81, fout);
                fputc((unsigned char)(0xC0 + r), fout);
                fputc((int)(v & 0xFF), fout);
                fputc((int)((v >> 8) & 0xFF), fout);
                fputc((int)((v >> 16) & 0xFF), fout);
                fputc((int)((v >> 24) & 0xFF), fout);
                continue;
            }
        } else {
            printf("linha %d: mnemonico?\n", ln);
            fails++;
            continue;
        }
        fputc((int)(v & 0xFF), fout);
        fputc((int)((v >> 8) & 0xFF), fout);
        fputc((int)((v >> 16) & 0xFF), fout);
        fputc((int)((v >> 24) & 0xFF), fout);
        cur += (strcmp(line, "add") == 0 && r != 0) ? 6 : 5;
        }
    }
    fclose(fin);
    fclose(fout);
    if (!fails) printf("oasm: %s ok\n", out);
    return fails ? 1 : 0;
}
