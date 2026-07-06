#include "asm.h"
#include "../sched/gdt.h"
#include "../idt/idt.h"

// Execute CPUID
void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf)
        : "memory"
    );
}

// Read MSR
uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ __volatile__(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
        : "memory"
    );
    return ((uint64_t)high << 32) | low;
}

// Write MSR
void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ __volatile__(
        "wrmsr"
        : 
        : "a"(low), "d"(high), "c"(msr)
        : "memory"
    );
}

void lgdt(struct gdt_ptr* gdt_ptr){
    __asm__ __volatile__ ("lgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void lidt(struct idt_ptr* idt_ptr){
    __asm__ __volatile__ ("lidt %0" : : "m" (*idt_ptr) : "memory");
}

void ltr(uint16_t tss_selector){
    __asm__ __volatile__("ltr %0" : : "r" (tss_selector) : "memory");
}

// Send 8 bits to a port
void io_outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ (
        "outb %0, %1" 
        : 
        : "a"(val), "Nd"(port) 
    );
}

// Receive 8 bits from a port
void io_wait(void){
    io_outb(0x80, 0);
}

uint8_t io_inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );
    return ret;
}