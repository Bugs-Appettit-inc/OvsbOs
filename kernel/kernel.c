/* ♥ OvsbMkM - Kernel com ring 3! "Agora com modo usuario~ kyun!"
 * Dica: TSS, GDT com segmentos ring 3, syscall int 0x80~
 * Processo idle (shell) roda em ring 0~ o programinha em ring 3!
 * Se der page fault, e porque o usuario e um baka! >_< */
#include <stdint.h>
#include "idt.h"
#include "memory.h"
#include "pit.h"
#include "serial.h"
#include "console.h"
#include "shell.h"
#include "tss.h"
#include "process.h"
#include "syscall.h"
#include "utils.h"
#include "drivers/ata.h"
#include "drivers/mouse.h"
#include "drivers/keyboard.h"
#include "fs/fat32.h"
#include "gui/vesa.h"
#include "wm/wm.h"
#include "apps_bin.h"

void keyboard_init(void);
void pic_init(void);

volatile int sigint_pending = 0;
framebuffer_t g_fb;
int g_fb_active = 0;

/* ♥ Endereco do programinha ring 3 (2MB ~ livre!) */
#define USER_PROG_ADDR ((void *)0x200000)
extern const int _binary_user_prog_size;

/* Pilha do usuario */
#define USER_STACK_SIZE 4096
static char user_stack[USER_STACK_SIZE] __attribute__((aligned(16)));

extern const uint8_t _binary_user_prog_start[];
void user_prog_launch(void) {
    console_write("Iniciando programa ring 3...\n");
    uint8_t *dst = (uint8_t *)USER_PROG_ADDR;
    for (int i = 0; i < _binary_user_prog_size; i++)
        dst[i] = _binary_user_prog_start[i];
    int pid = process_create_user("ring3test", USER_PROG_ADDR, user_stack, USER_STACK_SIZE);
    if (pid < 0) { console_write("Erro!\n"); return; }
    process_switch_to(pid);
    console_write("Programa ring 3 encerrou!\n");
}

/* ♥ Executa um app embutido pelo nome! */
void app_launch(const char *name) {
    for (int i = 0; i < NUM_APPS; i++) {
        if (strcmp(name, app_table[i].name) == 0) {
            console_printf("Iniciando app %s...\n", name);
            uint8_t *dst = (uint8_t *)USER_PROG_ADDR;
            for (unsigned int j = 0; j < app_table[i].size; j++)
                dst[j] = app_table[i].data[j];
            int pid = process_create_user(name, USER_PROG_ADDR, user_stack, USER_STACK_SIZE);
            if (pid < 0) { console_write("Erro!\n"); return; }
            process_switch_to(pid);
            console_printf("App %s encerrou!\n", name);
            return;
        }
    }
    console_write("App nao encontrado! Apps: ");
    for (int i = 0; i < NUM_APPS; i++) {
        console_write(app_table[i].name);
        console_write(" ");
    }
    console_write("\n");
}

void kmain(uint32_t magic, uint32_t mb_info) {
    serial_init();
    serial_puts("[OvsbMkM] Iniciando~ kyun!\r\n");
    idt_init(); pic_init(); idt_set_irq1();
    keyboard_init(); memory_init();
    ata_init();
    if (fat32_init() == 0) {
        console_write("FAT32: disco montado!\n");
    } else {
        console_write("FAT32: sem disco ou erro\n");
    }
    
    /* ♥ Inicializa FPU/SSE globalmente para todos os processos ring 3 */
    {
        uint64_t _cr0, _cr4;
        __asm__ volatile("mov %%cr0, %0" : "=r"(_cr0));
        _cr0 &= ~((uint64_t)0x0C);  // limpa TS (bit 3) e EM (bit 2)
        __asm__ volatile("mov %0, %%cr0" :: "r"(_cr0));
        __asm__ volatile("mov %%cr4, %0" : "=r"(_cr4));
        _cr4 |= 0x200;              // seta OSFXSR (bit 9)
        __asm__ volatile("mov %0, %%cr4" :: "r"(_cr4));
    }
__asm__ volatile("sti");
    pit_init();

    g_fb.addr = 0;
    uint32_t *tags = (uint32_t *)(uintptr_t)mb_info;
    if (magic == 0x36D76289 && tags) {
        uint32_t total = tags[0];
        for (uint32_t i = 8; i < total;) {
            uint32_t type = *(uint32_t *)((uint8_t *)tags + i);
            uint32_t size = *(uint32_t *)((uint8_t *)tags + i + 4);
            if (type == 8) {
                g_fb.addr  = *(uint64_t *)((uint8_t *)tags + i + 8);
                g_fb.pitch = *(uint32_t *)((uint8_t *)tags + i + 16);
                g_fb.width = *(uint32_t *)((uint8_t *)tags + i + 20);
                g_fb.height= *(uint32_t *)((uint8_t *)tags + i + 24);
                g_fb.bpp   = *(uint8_t *)((uint8_t *)tags + i + 28);
            }
            if (type == 0) break;
            i += size;
            if (i % 8) i += 8 - (i % 8);
        }
    }

    if (!g_fb.addr || g_fb.bpp != 32) {
        g_fb.addr  = 0xFD000000;
        g_fb.pitch = 1280 * 4;
        g_fb.width = 1280;
        g_fb.height = 720;
        g_fb.bpp = 32;
    }

    if (g_fb.addr && g_fb.bpp == 32) {
        if (vesa_init(&g_fb) == 0) {
            g_fb_active = 1;
            serial_puts("[OvsbMkM] VESA OK!\r\n");
        }
    }

    console_init();
    serial_puts("[OvsbMkM] Console pronto!\r\n");

    /* Ring 3 setup */
    tss_init();
    syscall_init();
    process_init();

    shell_init();

    /* ♥ Inicializa Window Manager */
    if (g_fb_active && g_fb.addr) {
        wm_init((uint32_t *)g_fb.addr, g_fb.width, g_fb.height, g_fb.pitch / 4);
        serial_puts("[WM] Window Manager pronto!\r\n");
    }

    /* ♥ OWT pronto! Digite 'owt' no shell pra testar */

    while (1) {
        if (keyboard_avail()) {
            char key = keyboard_read();
            shell_input(key);
        }
        __asm__ volatile("hlt");
    }
}
