
/* ♥ SHELL ~ console interativo! comandos, exec, ls, cd~ tudo aqui! */
#include "shell.h"
#include "console.h"
#include "memory.h"
#include "mach_o.h"
#include "process.h"
#include "gui/vesa.h"
#include "fs/fat32.h"
extern void app_launch(const char *name);
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static int strieq(const char *a, const char *b, int n);

#define MAX_CMD 256

static char cmd_buf[MAX_CMD];
static int  cmd_pos = 0;

extern framebuffer_t g_fb;
extern void user_prog_launch(void);
static void prompt(void) {
    console_write("ovsb> ");
}

static void cmd_help(void) {
    console_write("Comandos:\n");
    console_write("  help                  Mostra esta ajuda\n");
    console_write("  clear                 Limpa a tela\n");
    console_write("  echo <texto>          Imprime o texto\n");
    console_write("  info                  Info do sistema (VESA, heap)\n");
    console_write("  hexdump <addr>        Exibe 64 bytes do endereco\n");
    console_write("  run                   Executa programa ring 3 (embutido)\n");
    console_write("  exec <arquivo>        Carrega e executa Mach-O do FAT32\n");
    console_write("  ls                    Lista diretorio FAT32\n");
    console_write("  cd <dir>              Muda diretorio FAT32\n");
    console_write("  owttest               Executa demo OWT (ring 3, FAT32)\n");
    console_write("  owtapp                Executa app OWT (ring 3, FAT32)\n");
    console_write("  reboot                Reinicia o sistema\n");
}

static void cmd_clear(void) {
    console_clear();
}

static void cmd_echo(const char *args) {
    if (args) console_write(args);
    console_write("\n");
}

static void cmd_info(void) {
    console_printf("VESA: %dx%d %dbpp\n", g_fb.width, g_fb.height, g_fb.bpp);
    console_printf("Framebuffer: 0x%x\n", (unsigned int)g_fb.addr);
    console_printf("Pitch: %d\n", g_fb.pitch);
    console_printf("Heap: 64MB bump allocator\n");
}

static void cmd_hexdump(const char *args) {
    if (!args || !args[0]) { console_write("uso: hexdump <addr_hex>\n"); return; }
    unsigned int addr = 0;
    for (int i = 0; args[i] && args[i] != ' '; i++) {
        char c = args[i];
        if (c >= '0' && c <= '9') addr = addr * 16 + (c - '0');
        else if (c >= 'a' && c <= 'f') addr = addr * 16 + (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') addr = addr * 16 + (c - 'A' + 10);
        else break;
    }
    uint8_t *p = (uint8_t *)(uintptr_t)addr;
    for (int row = 0; row < 4; row++) {
        console_printf("%x: ", addr + row * 16);
        for (int i = 0; i < 16; i++) {
            char tmp[8];
            unsigned char b = p[row * 16 + i];
            tmp[0] = "0123456789abcdef"[b >> 4];
            tmp[1] = "0123456789abcdef"[b & 0xF];
            tmp[2] = ' ';
            tmp[3] = 0;
            console_write(tmp);
        }
        console_write(" |");
        for (int i = 0; i < 16; i++) {
            unsigned char b = p[row * 16 + i];
            char ch = (b >= 32 && b < 127) ? b : '.';
            console_putchar(ch);
        }
        console_write("|\n");
    }
}

static void cmd_run(void) {
    user_prog_launch();
}

#define EXEC_USER_STACK_SIZE 65536

static void cmd_exec(const char *args) {
    if (!args || !args[0]) {
        console_write("uso: exec <arquivo>\n");
        return;
    }
    uint32_t fsize;
    uint8_t attr;
    if (fat32_stat(args, &fsize, &attr, 0, 0) < 0) {
        console_write("exec: arquivo nao encontrado\n");
        return;
    }
    if (fsize < 32) {
        console_write("exec: arquivo muito pequeno\n");
        return;
    }
    uint8_t *buf = kmalloc(fsize + 1);
    if (!buf) {
        console_write("exec: sem memoria\n");
        return;
    }
    if (fat32_read_file(args, buf, fsize) < 0) {
        console_write("exec: erro de leitura\n");
        kfree(buf);
        return;
    }
    console_printf("exec: carregando %s (%d bytes)\n", args, (unsigned)fsize);
    void *entry = mach_o_load(buf, fsize);
    if (!entry) {
        console_write("exec: formato Mach-O invalido\n");
        kfree(buf);
        return;
    }
    void *ustack = kmalloc(EXEC_USER_STACK_SIZE);
    if (!ustack) {
        console_write("exec: sem memoria para pilha\n");
        kfree(buf);
        return;
    }
    int pid = process_create_user(args, entry, ustack, EXEC_USER_STACK_SIZE);
    if (pid < 0) {
        console_write("exec: erro ao criar processo\n");
        kfree(buf);
        kfree(ustack);
        return;
    }
    kfree(buf);
    console_printf("exec: PID %d rodando\n", pid);
    process_switch_to(pid);
    kfree(ustack);
    console_write("exec: processo encerrou\n");
}

static void cmd_owtapp(void) { cmd_exec("OWTAPP.BIN"); }

static void cmd_owttest(void) {
    cmd_exec("OWTTEST.BIN");
}

static void cmd_desktop(void) { app_launch("desktop"); }
static void cmd_app(const char *args) {
    if (!args || !args[0]) {
        console_write("uso: app <nome>\n");
        console_write("apps: desktop, hello\n");
        return;
    }
    app_launch(args);
}

static void cmd_ls(void) {
    fat32_list_dir();
}

static void cmd_cd(const char *args) {
    if (!args || !args[0]) {
        fat32_change_dir("..");
        return;
    }
    fat32_change_dir(args);
    char cwd[256];
    fat32_get_cwd_name(cwd, 256);
    console_write(cwd);
    console_write("\n");
}

static void cmd_reboot(void) {
    console_write("Reiniciando...\n");
    outb(0x64, 0xFE);
    for(;;);
}

static void execute(const char *cmd) {
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    const char *args = cmd;
    while (*args && *args != ' ') args++;
    int cmd_len = args - cmd;
    while (*args == ' ') args++;

    if (strieq(cmd, "help", 4) && cmd_len == 4) cmd_help();
    else if (strieq(cmd, "clear", 5) && cmd_len == 5) cmd_clear();
    else if (strieq(cmd, "echo", 4) && cmd_len == 4) cmd_echo(args);
    else if (strieq(cmd, "info", 4) && cmd_len == 4) cmd_info();
    else if (strieq(cmd, "hexdump", 7) && cmd_len == 7) cmd_hexdump(args);
    else if (strieq(cmd, "run", 3) && cmd_len == 3) cmd_run();
    else if (strieq(cmd, "exec", 4) && cmd_len == 4) cmd_exec(args);
    else if (strieq(cmd, "ls", 2) && cmd_len == 2) cmd_ls();
    else if (strieq(cmd, "cd", 2) && cmd_len == 2) cmd_cd(args);
        else if (strieq(cmd, "owtapp", 6) && cmd_len == 6) cmd_owtapp();
    else if (strieq(cmd, "owttest", 7) && cmd_len == 7) cmd_owttest();
    else if (strieq(cmd, "desktop", 7) && cmd_len == 7) cmd_desktop();
    else if (strieq(cmd, "app", 3) && cmd_len == 3) cmd_app(*args ? args : "");
    else if (strieq(cmd, "owt", 3) && cmd_len == 3) cmd_owttest();
    else if (strieq(cmd, "reboot", 6) && cmd_len == 6) cmd_reboot();
    else {
        console_write("comando nao encontrado: ");
        console_write(cmd);
        console_write("\n");
    }
}

static int strieq(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
    return 1;
}

void shell_init(void) {
    cmd_pos = 0;
    console_write("OvsbMkM Kernel Console\n");
    prompt();
}

void shell_input(char c) {
    if (c == '\n') {
        cmd_buf[cmd_pos] = 0;
        console_write("\n");
        execute(cmd_buf);
        cmd_pos = 0;
        prompt();
    } else if (c == '\b') {
        if (cmd_pos > 0) {
            cmd_pos--;
            console_write("\b \b");
        }
    } else {
        if (cmd_pos < MAX_CMD - 1) {
            cmd_buf[cmd_pos++] = c;
            console_putchar(c);
        }
    }
}

/* ♥ TODO: adicionar suporte a argumentos pra exec ~ argv futuramente! */

/* ♥ HELLO ~ kernel funcionando! */
