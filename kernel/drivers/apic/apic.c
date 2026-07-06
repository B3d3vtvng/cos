#include "apic.h"

extern void isr_siv(void);
extern void __attribute__((noreturn)) isr_apic_err(void);

static uint64_t lapic_base;

#define X2APIC_LVT_TIMER 0x832
#define X2APIC_TIMER_ICR 0x833
#define X2APIC_TIMER_CCR 0x834
#define X2APIC_TIMER_DCR 0x83D

static bool lapic_use_x2apic(void){
    return (rdmsr(0x1B) & (1ULL << 10)) != 0;
}

void lapic_write(uint32_t reg, uint32_t val){
    volatile uint32_t* reg_addr = (uint32_t*) (lapic_base + reg);
    *reg_addr = val;
}

uint32_t lapic_read(uint32_t reg){
    volatile uint32_t* reg_addr = (uint32_t*) (lapic_base + reg);
    return *reg_addr;
}

void lapic_timer_write(uint32_t reg, uint32_t val){
    if (lapic_use_x2apic()) {
        switch (reg) {
        case LAPIC_LVT_TIMER_REG_OFF: wrmsr(X2APIC_LVT_TIMER, val); return;
        case LAPIC_TIMER_ICR_OFF: wrmsr(X2APIC_TIMER_ICR, val); return;
        case LAPIC_TIMER_DCR_OFF: wrmsr(X2APIC_TIMER_DCR, val); return;
        default: break;
        }
    }
    lapic_write(reg, val);
}

uint32_t lapic_timer_read(uint32_t reg){
    if (lapic_use_x2apic()) {
        switch (reg) {
        case LAPIC_LVT_TIMER_REG_OFF: return (uint32_t)rdmsr(X2APIC_LVT_TIMER);
        case LAPIC_TIMER_ICR_OFF: return (uint32_t)rdmsr(X2APIC_TIMER_ICR);
        case LAPIC_TIMER_CCR_OFF: return (uint32_t)rdmsr(X2APIC_TIMER_CCR);
        case LAPIC_TIMER_DCR_OFF: return (uint32_t)rdmsr(X2APIC_TIMER_DCR);
        default: break;
        }
    }
    return lapic_read(reg);
}

bool local_apic_exists(void){
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // Bit 9 of EDX = Local APIC present
    return (edx & (1 << 9)) != 0;
}

void local_apic_enable(void){
    uint64_t lapic_msr = rdmsr(0x1B);
    lapic_msr &= ~(1ULL << 10);
    lapic_msr |= (1ULL << 11);
    wrmsr(0x1B, lapic_msr);

    if (rdmsr(0x1B) & (1ULL << 10)) {
        kernel_panic(NULL, "Failed to disable x2APIC mode\n");
    }
}

uint64_t get_lapic_phys_base(void){
    return rdmsr(0x1B) & 0xFFFFFFFFFFFFF000ULL;
}

uint64_t remap_lapic(uint64_t phys){
    uint64_t virt = (uint64_t)alloc_virt(1);
    map_virtual(get_pgtable(), virt, phys, P_KERNEL | P_PRESENT | P_WRITABLE | P_PWT | P_PCD, PG_SIZE_REG);
    return virt;
}

void lapic_send_self_ipi(uint8_t vector){
    if (lapic_use_x2apic()) {
        wrmsr(0x830, vector | (1ULL << 18));
        return;
    }
    lapic_write(LAPIC_ICR_OFF + 0x10, 0);
    lapic_write(LAPIC_ICR_OFF, vector | (1ULL << 18));
}

void mask_pic(void){
    io_outb(0x21, 0xFF);
    io_outb(0xA1, 0xFF);
}

void pic_remap(uint8_t master_vector, uint8_t slave_vector){
    uint8_t master_mask = io_inb(0x21);
    uint8_t slave_mask = io_inb(0xA1);

    io_outb(0x20, 0x11);
    io_outb(0xA0, 0x11);
    io_outb(0x21, master_vector);
    io_outb(0xA1, slave_vector);
    io_outb(0x21, 0x04);
    io_outb(0xA1, 0x02);
    io_outb(0x21, 0x01);
    io_outb(0xA1, 0x01);

    io_outb(0x21, master_mask);
    io_outb(0xA1, slave_mask);
}

void lapic_enable_pic_passthrough(void){
    lapic_write(LAPIC_LVT_LINT0_REG_OFF, 0x700);
}

void pic_unmask_irq0(void){
    io_outb(0x21, io_inb(0x21) & ~1);
}

void pic_eoi_master(void){
    io_outb(0x20, 0x20);
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
void init_apic(struct idt_ptr* idt_ptr){
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

    // Mask Legacy interrupts
    lapic_write(LAPIC_LVT_LINT0_REG_OFF, 0x10000);
    lapic_write(LAPIC_LVT_LINT1_REG_OFF, 0x10000);

    // Map Error interrupts to interrupt vector 0xFE
    lapic_write(LAPIC_LVT_ER_REG_OFF, 0xFE);

    // Register SIV handler to 0xFF
    idt_set_gate((struct idt_entry*) idt_ptr->base, 0xFF, (uint64_t)isr_siv, 0x08, 0x8E);

    // Register ERR handler to 0xFE
    idt_set_gate((struct idt_entry*) idt_ptr->base, 0xFE, (uint64_t)isr_apic_err, 0x08, 0x8E);

    // Clear all previous interrupts (if any)
    lapic_write(LAPIC_EOI_REG_OFF, 0);

    kprintf("Initialized LAPIC software components\n");
}