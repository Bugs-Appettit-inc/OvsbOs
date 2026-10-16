/* chip8 — emulador CHIP-8 (~250 linhas): CPU + display 64x32 + input.
 * TUI: desenha com blocos ASCII; teclas 1234/qwer/asdf/zxcv = hex 1-F.
 * --gui: framebuffer OvsbOS. --cycles N: passos p/ teste. ROMs livres em roms/.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../ovsb_fb.h"

static unsigned char mem[4096];
static unsigned char V[16];
static unsigned short I, pc, stack[16];
static unsigned char sp, dt, st;
static unsigned char gfx[64 * 32];
static unsigned short keys;
static unsigned long rng = 0x12345678;

static const unsigned char fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
};

static void c8_reset(void) {
    memset(mem, 0, sizeof(mem));
    memset(V, 0, sizeof(V));
    memset(gfx, 0, sizeof(gfx));
    memset(stack, 0, sizeof(stack));
    memcpy(mem, fontset, sizeof(fontset));
    I = 0;
    pc = 0x200;
    sp = 0;
    dt = st = 0;
    keys = 0;
}

static int c8_load(const char *path) {
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return -1;
    n = fread(mem + 0x200, 1, 4096 - 0x200, f);
    fclose(f);
    return n > 0 ? 0 : -1;
}

static unsigned rnd15(void) {
    rng = rng * 1103515245 + 12345;
    return (unsigned)((rng >> 16) & 0x7FFF);
}

/* executa 1 opcode; retorna -1 em instrucao invalida/retorno vazio. */
static int c8_step(void) {
    unsigned op, x, y, n, nnn;
    if (pc + 1 >= 4096) return -1;
    op = ((unsigned)mem[pc] << 8) | mem[pc + 1];
    pc += 2;
    x = (op >> 8) & 0xF;
    y = (op >> 4) & 0xF;
    n = op & 0xF;
    nnn = op & 0xFFF;
    switch (op & 0xF000) {
    case 0x0000:
        if (op == 0x00E0) memset(gfx, 0, sizeof(gfx));
        else if (op == 0x00EE) {
            if (sp == 0) return -1;
            pc = stack[--sp];
        } else return -1;
        break;
    case 0x1000: pc = (unsigned short)nnn; break;
    case 0x2000:
        if (sp >= 16) return -1;
        stack[sp++] = pc;
        pc = (unsigned short)nnn;
        break;
    case 0x3000: if (V[x] == (op & 0xFF)) pc += 2; break;
    case 0x4000: if (V[x] != (op & 0xFF)) pc += 2; break;
    case 0x5000:
        if (n != 0) return -1;
        if (V[x] == V[y]) pc += 2;
        break;
    case 0x6000: V[x] = (unsigned char)(op & 0xFF); break;
    case 0x7000: V[x] = (unsigned char)(V[x] + (op & 0xFF)); break;
    case 0x8000:
        switch (n) {
        case 0: V[x] = V[y]; break;
        case 1: V[x] |= V[y]; break;
        case 2: V[x] &= V[y]; break;
        case 3: V[x] ^= V[y]; break;
        case 4: {
            unsigned s = (unsigned)V[x] + V[y];
            V[0xF] = s > 255 ? 1 : 0;
            V[x] = (unsigned char)s;
            break;
        }
        case 5:
            V[0xF] = V[x] >= V[y] ? 1 : 0;
            V[x] = (unsigned char)(V[x] - V[y]);
            break;
        case 6:
            V[0xF] = (unsigned char)(V[x] & 1);
            V[x] >>= 1;
            break;
        case 7:
            V[0xF] = V[y] >= V[x] ? 1 : 0;
            V[x] = (unsigned char)(V[y] - V[x]);
            break;
        case 0xE:
            V[0xF] = (unsigned char)((V[x] >> 7) & 1);
            V[x] = (unsigned char)(V[x] << 1);
            break;
        default: return -1;
        }
        break;
    case 0x9000:
        if (n != 0) return -1;
        if (V[x] != V[y]) pc += 2;
        break;
    case 0xA000: I = (unsigned short)nnn; break;
    case 0xB000: pc = (unsigned short)(nnn + V[0]); break;
    case 0xC000: V[x] = (unsigned char)(rnd15() & (op & 0xFF)); break;
    case 0xD000: {
        unsigned r, px;
        V[0xF] = 0;
        for (r = 0; r < n; r++) {
            unsigned char spr = (I + r < 4096) ? mem[I + r] : 0;
            int b;
            for (b = 0; b < 8; b++) {
                if (!(spr & (0x80 >> b))) continue;
                px = ((V[x] + b) % 64) + ((V[y] + r) % 32) * 64;
                if (gfx[px]) V[0xF] = 1;
                gfx[px] ^= 1;
            }
        }
        break;
    }
    case 0xE000:
        if ((op & 0xFF) == 0x9E) {
            if (V[x] < 16 && (keys & (1u << V[x]))) pc += 2;
        } else if ((op & 0xFF) == 0xA1) {
            if (V[x] >= 16 || !(keys & (1u << V[x]))) pc += 2;
        } else return -1;
        break;
    case 0xF000:
        switch (op & 0xFF) {
        case 0x07: V[x] = dt; break;
        case 0x0A: {
            int k;
            /* bloqueante simplificado: primeira tecla ou 0 */
            V[x] = 0;
            for (k = 0; k < 16; k++)
                if (keys & (1 << k)) { V[x] = (unsigned char)k; break; }
            break;
        }
        case 0x15: dt = V[x]; break;
        case 0x18: st = V[x]; break;
        case 0x1E: I = (unsigned short)(I + V[x]); break;
        case 0x29: I = (unsigned short)(V[x] * 5); break;
        case 0x33:
            if (I + 2 >= 4096) return -1;
            mem[I] = (unsigned char)(V[x] / 100);
            mem[I + 1] = (unsigned char)((V[x] / 10) % 10);
            mem[I + 2] = (unsigned char)(V[x] % 10);
            break;
        case 0x55: {
            unsigned k;
            if (I + x >= 4096) return -1;
            for (k = 0; k <= x; k++) mem[I + k] = V[k];
            break;
        }
        case 0x65: {
            unsigned k;
            if (I + x >= 4096) return -1;
            for (k = 0; k <= x; k++) V[k] = mem[I + k];
            break;
        }
        default: return -1;
        }
        break;
    default: return -1;
    }
    if (dt > 0) dt--;
    if (st > 0) st--;
    return 0;
}

static void tui_draw(void) {
    int x, y;
    printf("\033[H\033[2J");
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 64; x++)
            printf("%s", gfx[y * 64 + x] ? "##" : "  ");
        printf("\n");
    }
    printf("pc=%03x I=%03x dt=%d (1234/qwer/asdf/zxcv, q=sai)\n", pc, I, dt);
    fflush(stdout);
}

/* mapa tecla ASCII -> hex CHIP-8 */
static int keymap(int c) {
    switch (c) {
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 0xC;
    case 'q': return -2;
    case 'w': return 4;
    case 'e': return 5;
    case 'r': return 6;
    case 'a': return 7;
    case 's': return 8;
    case 'd': return 9;
    case 'f': return 0xE;
    case 'z': return 0xA;
    case 'x': return 0;
    case 'c': return 0xB;
    case 'v': return 0xF;
    default: return -1;
    }
}

static void gui_run(const char *rom) {
    app_fb_t fb;
    int quit = 0, steps = 0;
    (void)rom;
    if (!app_fb_init(&fb)) {
        printf("sem framebuffer (OVSBOS=1 no guest)\n");
        return;
    }
    while (!quit) {
        int k = app_kbd_get(), m, x, y;
        unsigned long sc = fb.w / 64, sy = fb.h / 32;
        if (sc < 1) sc = 1;
        if (sy < 1) sy = 1;
        if (k == 'q' || k == APP_KEY_ESC) quit = 1;
        else if (k >= 0 && k < 256) {
            int h = keymap(k);
            if (h >= 0) keys |= (unsigned short)(1 << h);
        }
        for (m = 0; m < 8; m++)
            if (c8_step() != 0) { quit = 1; break; }
        keys = 0;
        steps++;
        app_rect(&fb, 0, 0, fb.w, fb.h, 0xFF101418);
        for (y = 0; y < 32; y++)
            for (x = 0; x < 64; x++)
                if (gfx[y * 64 + x])
                    app_rect(&fb, (unsigned long)x * sc,
                             (unsigned long)y * sy, sc, sy, 0xFF33FF66);
        app_fb_flush(&fb);
        usleep(16000); /* ~60fps sem torrar CPU */
        (void)steps;
    }
}

int main(int argc, char **argv) {
    const char *rom = NULL;
    long cycles = -1;
    int gui = 0, i, bad = 0, dump = 0;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gui") == 0) gui = 1;
        else if (strcmp(argv[i], "--dump") == 0) dump = 1;
        else if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc)
            cycles = atol(argv[++i]);
        else if (argv[i][0] != '-')
            rom = argv[i];
    }
    if (!rom) {
        printf("uso: chip8 ROM [--gui] [--cycles N]\n");
        return 1;
    }
    c8_reset();
    if (c8_load(rom) != 0) {
        printf("rom? %s\n", rom);
        return 1;
    }
    if (gui) { gui_run(rom); return 0; }
    if (cycles >= 0) {
        long k;
        for (k = 0; k < cycles; k++)
            if (c8_step() != 0) {
                printf("halt pc=%03x step=%ld\n", pc, k);
                return 2;
            }
        if (dump) {
            int p, n = 0;
            for (p = 0; p < 64 * 32; p++)
                if (gfx[p]) n++;
            printf("pixels=%d\n", n);
        }
        printf("ok %ld ciclos pc=%03x\n", cycles, pc);
        return 0;
    }
    /* TUI interativa: stdin nao-bloqueante via leitura simples */
    printf("chip8: %s (q sai)\n", rom);
    for (;;) {
        int c, m;
        tui_draw();
        for (m = 0; m < 9; m++)
            if (c8_step() != 0) {
                printf("halt pc=%03x\n", pc);
                return 0;
            }
        keys = 0;
        c = getchar();
        if (c == EOF) break;
        {
            int h = keymap(c);
            if (h == -2) break;
            if (h >= 0) keys |= (unsigned short)(1 << h);
            else if (c == '\n' || c == ' ') keys = 0;
            else bad = c;
        }
        (void)bad;
    }
    return 0;
}
