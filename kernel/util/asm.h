#ifndef ASM_H
#define ASM_H

#include <stdint.h>
#include <stddef.h>

void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);
void lgdt(uint64_t gdt_ptr);
void ltr(uint16_t tss_desc);
void io_outb(uint16_t port, uint8_t val);
uint8_t io_inb(uint16_t port);

#endif