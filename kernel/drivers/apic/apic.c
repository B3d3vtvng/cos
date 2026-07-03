#include "apic.h"

extern void isr_siv(void);
extern void __attribute__((noreturn)) isr_apic_err(void);

static uint64_t lapic_base;

void lapic_write(uint32_t reg, uint32_t val){
    volatile uint32_t* reg_addr = (uint32_t*) (lapic_base + reg);
    *reg_addr = val;
}

uint32_t lapic_read(uint32_t reg){
    volatile uint32_t* reg_addr = (uint32_t*) (lapic_base + reg);
    return *reg_addr;
}

bool local_apic_exists(void){
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // Bit 9 of EDX = Local APIC present
    return (edx & (1 << 9)) != 0;
}

void local_apic_enable(void){
    uint64_t LAPIC_base = rdmsr(0x1B);
    LAPIC_base |= (1ULL << 11);
    wrmsr(0x1B, LAPIC_base);
}

uint64_t get_lapic_phys_base(void){
    return rdmsr(0x1B) & 0xFFFFFFFFFFFFF000ULL;
}

uint64_t remap_lapic(uint64_t phys){
    uint64_t virt = (uint64_t)alloc_virt(1);
    map_virtual(get_pgtable(), virt, phys, P_KERNEL | P_PRESENT | P_WRITABLE | P_PWT | P_PCD, PG_SIZE_REG);
    return virt;
}

void mask_pic(void){
    // Mask the PIC master port
    io_outb(0x21, 0xFF);

    // Mask the PIC slave port
    io_outb(0xA1, 0xFF);
}

void isr_apic_err_handler(void){
    lapic_write(LAPIC_ESR_OFF, 0);
    uint32_t err = lapic_read(LAPIC_ESR_OFF);
    char err_msg[40];
    memset(err_msg, 0, 40);
    ksnprintf(err_msg, 40, "Encountered LAPIC error: 0x%udh\n", err);

    kernel_panic(NULL, err_msg);

    __builtin_unreachable();
}

// Initialize the APIC driver
void init_apic(struct idt_ptr idt_ptr){
    if (!local_apic_exists()){
        kernel_panic(NULL, "Local APIC does not exist\n");
    }

    local_apic_enable();
    uint64_t phys_base = (uint64_t)get_lapic_phys_base();
    uint64_t virt_base = remap_lapic(phys_base);

    lapic_base = virt_base;

    kprintf("Enabled and remapped lapic: phys: 0x%llx, virt: 0x%llx\n", phys_base, virt_base);

    mask_pic();

    // Enable the Spurious Interrupt vector register and map to 0xFF
    lapic_write(LAPIC_SIVR_OFF, (LAPIC_SIVR_SOFTWARE_ENABLE | 0xFF));

    // Clear the Task Priority Register
    lapic_write(LAPIC_TPR_OFF, 0);

    // Initialize the DFR to 0xFFFFFFFF (Flat mode)
    lapic_write(LAPIC_DFR_OFF, 0xFFFFFFFF);

    // Initialize the LDR to logical id 1
    lapic_write(LAPIC_LDR_OFF, (1 << 24));

    // Mask Legacy interrupts
    lapic_write(LAPIC_LVT_LINT0_REG_OFF, 0x10000);
    lapic_write(LAPIC_LVT_LINT1_REG_OFF, 0x10000);

    // Map Error interrupts to interrupt vector 0xFE
    lapic_write(LAPIC_LVT_ER_REG_OFF, 0xFE);

    // Register SIV handler to 0xFF
    idt_set_gate((struct idt_entry*) idt_ptr.base, 0xFF, (uint64_t)isr_siv, 0x08, 0x8E);

    // Register ERR handler to 0xFE
    idt_set_gate((struct idt_entry*) idt_ptr.base, 0xFE, (uint64_t)isr_apic_err, 0x08, 0x8E);

    // Clear all previous interrupts (if any)
    lapic_write(LAPIC_EOI_REG_OFF, 0);

    kprintf("Initialized LAPIC software components\n");
}