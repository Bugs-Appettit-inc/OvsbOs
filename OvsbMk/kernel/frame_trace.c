/* frame_trace.c - anel de frames do syscall para diagnosticar o +0x68
 *
 * Cada entrada (64 bytes): tsc(8) pid(4) tag(4) f0..f5(48).
 * Tags: 1 = save (entrada do syscall), 2 = restore (antes do iretq),
 *       3 = irq0 executando em ring 0 (interrompeu o kernel).
 *
 * frame_dump() e chamado do #PF quando a assinatura do +0x68 eh vista
 * (CS de ring 3 + err com bit US, ou seja, fetch de pagina do supervisor
 * vindo do usuario). O anel mostra qual slot do frame diverge e se um
 * irq0 interrompeu o kernel exatamente na janela de montagem do iretq.
 */

#include <stdint.h>
#include "serial.h"
#include "process.h"

#define FRAME_RB_N 32

struct frame_evt {
    uint64_t tsc;
    uint32_t pid;
    uint32_t tag;
    uint64_t f[6];
};

struct frame_evt frame_rb[FRAME_RB_N];
volatile uint64_t frame_w = 0;

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void frame_trace(uint32_t tag, uint64_t f0, uint64_t f1, uint64_t f2,
                 uint64_t f3, uint64_t f4, uint64_t f5) {
    uint64_t w = frame_w;
    frame_w = w + 1;
    struct frame_evt *e = &frame_rb[w & (FRAME_RB_N - 1)];
    e->tsc = rdtsc();
    e->pid = (uint32_t)process_current_pid();
    e->tag = tag;
    e->f[0] = f0; e->f[1] = f1; e->f[2] = f2;
    e->f[3] = f3; e->f[4] = f4; e->f[5] = f5;
}

void frame_dump(uint64_t rip, uint64_t cr2, uint64_t err) {
    serial_puts("\r\n[FRAME_TRACE] rip=");
    serial_puthex((uint32_t)rip);
    serial_puts(" cr2=");
    serial_puthex((uint32_t)cr2);
    serial_puts(" err=");
    serial_puthex((uint32_t)err);
    serial_puts(" w=");
    serial_puthex((uint32_t)frame_w);
    serial_puts("\r\n");
    uint64_t w = frame_w;
    uint64_t start = w > 8 ? w - 8 : 0;
    for (uint64_t k = w; k-- > start; ) {
        struct frame_evt *e = &frame_rb[k & (FRAME_RB_N - 1)];
        serial_puts("  t=");
        serial_puthex((uint32_t)e->tag);
        serial_puts(" p=");
        serial_puthex(e->pid);
        serial_puts(" f0=");
        serial_puthex((uint32_t)(e->f[0] >> 32));
        serial_puthex((uint32_t)e->f[0]);
        serial_puts(" f1=");
        serial_puthex((uint32_t)(e->f[1] >> 32));
        serial_puthex((uint32_t)e->f[1]);
        serial_puts(" f2=");
        serial_puthex((uint32_t)(e->f[2] >> 32));
        serial_puthex((uint32_t)e->f[2]);
        serial_puts(" f3=");
        serial_puthex((uint32_t)(e->f[3] >> 32));
        serial_puthex((uint32_t)e->f[3]);
        serial_puts(" f4=");
        serial_puthex((uint32_t)(e->f[4] >> 32));
        serial_puthex((uint32_t)e->f[4]);
        serial_puts(" f5=");
        serial_puthex((uint32_t)(e->f[5] >> 32));
        serial_puthex((uint32_t)e->f[5]);
        serial_puts("\r\n");
    }
}
