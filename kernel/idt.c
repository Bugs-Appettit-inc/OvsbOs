/* ♥ IDT - Interrupt Descriptor Table ~ "Tabela dos sonhos!"
 * Dica: se a IDT nao funcionar, suas IRQs vao chorar~ 
 * Cada entrada tem 16 bytes, nao confunda com a de 8!
 * Agora com syscall gate pra ring 3 ~ int 0x80 liberado! kyun~ <3 */
#include "idt.h"
#include "serial.h"

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void irq0(void);
extern void keyboard_irq_handler(void);
extern void syscall_isr(void);

void idt_set_entry(int num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = handler & 0xFFFF;
    idt[num].selector    = selector;
    idt[num].ist         = 0;
    idt[num].type_attr   = flags;
    idt[num].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].reserved    = 0;
}

void idt_init(void) {
    uint64_t handlers[] = {
        (uint64_t)isr0,  (uint64_t)isr1,  (uint64_t)isr2,  (uint64_t)isr3,
        (uint64_t)isr4,  (uint64_t)isr5,  (uint64_t)isr6,  (uint64_t)isr7,
        (uint64_t)isr8,  (uint64_t)isr9,  (uint64_t)isr10, (uint64_t)isr11,
        (uint64_t)isr12, (uint64_t)isr13, (uint64_t)isr14, (uint64_t)isr15,
        (uint64_t)isr16, (uint64_t)isr17, (uint64_t)isr18, (uint64_t)isr19,
        (uint64_t)isr20, (uint64_t)isr21, (uint64_t)isr22, (uint64_t)isr23,
        (uint64_t)isr24, (uint64_t)isr25, (uint64_t)isr26, (uint64_t)isr27,
        (uint64_t)isr28, (uint64_t)isr29, (uint64_t)isr30, (uint64_t)isr31
    };
    for (int i = 0; i < 32; i++)
        idt_set_entry(i, handlers[i], 0x08, IDT_PRESENT | IDT_INT_GATE);
    idt_set_entry(IRQ0, (uint64_t)irq0, 0x08, IDT_PRESENT | IDT_INT_GATE);
    for (int i = 33; i < IDT_ENTRIES; i++) {
        if (i == 128) continue;
        idt_set_entry(i, (uint64_t)irq0, 0x08, IDT_PRESENT | IDT_INT_GATE);
    }
    /* ♥ Syscall gate: trap gate (IF nao desligado) + DPL=3 pra ring 3 */
    idt_set_entry(128, (uint64_t)syscall_isr, 0x08,
                  IDT_PRESENT | IDT_TRAP_GATE | 0x60);
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint64_t)&idt;
    __asm__ volatile ("lidt %0" :: "m"(idt_ptr));
}

void idt_set_irq1(void) {
    idt_set_entry(IRQ1, (uint64_t)keyboard_irq_handler, 0x08,
                  IDT_PRESENT | IDT_INT_GATE);
}

void idt_handler(uint64_t *regs) {
    uint64_t int_num = regs[16];  /* número da interrupção */
    uint64_t err_code = regs[17]; /* código de erro */
    uint64_t rip = regs[15];      /* RIP no momento da exceção */
    
    serial_puts("\r\n[EXC] Excecao ");
    /* Imprime número como hex simples */
    char hex[] = "0123456789ABCDEF";
    serial_putc(hex[(int_num >> 4) & 0xF]);
    serial_putc(hex[int_num & 0xF]);
    serial_puts(" RIP=");
    for (int i = 60; i >= 0; i -= 4) {
        serial_putc(hex[(rip >> i) & 0xF]);
    }
    serial_puts(" ERR=");
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(hex[(err_code >> i) & 0xF]);
    }
    serial_puts("\r\n");
    
    /* Se for page fault (14), mostra o endereço que causou */
    if (int_num == 14) {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        serial_puts("[PF] CR2=");
        for (int i = 60; i >= 0; i -= 4) {
            serial_putc(hex[(cr2 >> i) & 0xF]);
        }
        serial_puts("\r\n");
    }
    
    __asm__ volatile("cli; hlt");
}

/* ♥ idt.c ~ arquivo fofinho do OvsbMkM! kyun~ <3 */
