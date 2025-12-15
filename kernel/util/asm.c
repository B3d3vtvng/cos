#include "asm.h"

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