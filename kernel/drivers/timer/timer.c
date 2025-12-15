#include "timer.h"

bool local_apic_exists(void){
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // Bit 9 of EDX = Local APIC present
    return (edx & (1 << 9)) != 0;
}

void local_apic_enable(void){
    uint64_t apic_base = rdmsr(0x1B);
    apic_base |= (1ULL << 11);
    wrmsr(0x1B, apic_base);
}

uint64_t get_apic_phys_base(void){
    return rdmsr(0x1B) & 0xFFFFFFFFFFFFF000ULL;
}

uint64_t remap_apic(uint64_t phys){
    uint64_t virt = (uint64_t)alloc_virt(1);
    map_virtual(get_pgtable(), virt, phys, P_KERNEL | P_PRESENT | P_WRITABLE | P_PWT | P_PCD, PG_SIZE_REG);
    return virt;
}

// Initialize the PIT timer driver
void init_timer_driver(void){
    if (!local_apic_exists()){
        kernel_panic(NULL, "Local APIC does not exist");
    }

    local_apic_enable();
    uint64_t phys_base = (uint64_t)get_apic_phys_base();
    uint64_t virt_base = remap_apic(phys_base);

    kprintf("Enabled and remapped apic: phys: 0x%llx, virt 0x%llx\n", phys_base, virt_base);
}