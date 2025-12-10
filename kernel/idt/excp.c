#include "idt.h"

// Round up to the nearest page boundary
#define ROUND_PG(x) (((x) + 0xFFF) & ~0xFFF)

static char* stack_guard_page = NULL;

// A map of exception messages, indexed by exception number
const char* excp_msg_map[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception"
};

// Declaration of ISR stub table, externally defined in isr.asm
extern void* isr_stub_table[];

// IDT pointer
struct idt_ptr idt_pointer;

// Set an entry in the IDT
void idt_set_gate(struct idt_entry* idt_entries, int n, uint64_t handler, uint16_t selector, uint8_t flags){
    idt_entries[n].offset_low = handler & 0xFFFF;
    idt_entries[n].selector = selector;
    idt_entries[n].ist = 0;
    idt_entries[n].type_attr = flags;
    idt_entries[n].offset_mid = (handler >> 16) & 0xFFFF;
    idt_entries[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt_entries[n].zero = 0;
}

// Initialize the IDT
struct idt_ptr idt_init(void){
    void* idt_entries = kmalloc(sizeof(struct idt_entry) * 256);
    // Set up IDT entries for CPU exceptions (0-31)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(idt_entries, i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }

    idt_pointer.limit = (uint16_t)(sizeof(struct idt_entry) * 256 - 1);
    idt_pointer.base = (uint64_t)idt_entries;

    __asm__ __volatile__ ("lidt %0" : : "m"(idt_pointer));
    return idt_pointer;
}

// Common handler function for all hardware exceptions
void isr_common_handler(struct excp_frame* excp_state){
    //TODO: Implement actual exception handlers
    kernel_panic(excp_state, excp_msg_map[excp_state->int_no]);
}

__attribute__((noreturn))
// Unrecoverable kernel panic handler
void kernel_panic(struct excp_frame* register_state, const char* message) {
    __asm__ volatile ("cli"); // disable interrupts

    vga_clear();
    vga_print("KERNEL PANIC\n");
    vga_print(message);

    if (register_state == NULL){
        vga_print("System halted");
        while (1) __asm__ __volatile__ ("hlt");
    }

    vga_print("\n\nCore dump:\n\n");

    char buffer[32];
    for (int i = 0; i < 32; i++)
        buffer[i] = '\0';

    // Column width (80-char VGA line / 2 columns)
    const int col_width = 38;

    #define REPORT_REG(reg) \
        vga_print(#reg ": "); \
        ltohex(register_state->reg, buffer); \
        vga_print(buffer);

    #define PRINT2(a,b) do { \
        REPORT_REG(a); \
        int len_left = strlen(#a) + 2 + strlen(buffer); /* name + '0x' + value */ \
        for (int i = len_left; i < col_width; i++) vga_print(" "); \
        REPORT_REG(b); \
        vga_print("\n"); \
    } while (0)

    PRINT2(rax, rbx);
    PRINT2(rcx, rdx);
    PRINT2(rsi, rdi);
    PRINT2(rbp, rsp);
    PRINT2(r8,  r9);
    PRINT2(r10, r11);
    PRINT2(r12, r13);
    PRINT2(r14, r15);
    PRINT2(rip, cs);
    PRINT2(rflags, ss);
    PRINT2(int_no, err_code);

    if (register_state && register_state->int_no == 14){
        kprintf("\nFaulting address: 0x%llx\n", get_cr2());

        if (stack_guard_page != NULL && 
            (get_cr2() >= (uint64_t)stack_guard_page && 
             get_cr2() < (uint64_t)(stack_guard_page + 0x1000))) {
            kprintf("Kernel Stack overflow detected (accessed guard page at 0x%llx)!\n", get_cr2());
        }
    }

    vga_print("\nSystem halted.\n");

    while (1) __asm__ volatile ("hlt");
}

