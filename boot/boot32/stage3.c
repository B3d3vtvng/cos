#include <stdint.h>

//###########################
// GDT Setup
//###########################

struct gdt_entry {
    uint16_t limit_low;     // Bits 0-15 of limit
    uint16_t base_low;      // Bits 0-15 of base
    uint8_t  base_middle;   // Bits 16-23 of base
    uint8_t  access;        // Access flags
    uint8_t  granularity;   // Flags + bits 16-19 of limit
    uint8_t  base_high;     // Bits 24-31 of base
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;          // 64-bit base for lgdt
} __attribute__((packed));

struct gdt_entry gdt64[3];
struct gdt_ptr gdt64_ptr;

void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t gran) {
    gdt64[idx].limit_low    = (limit & 0xFFFF);
    gdt64[idx].base_low     = (base & 0xFFFF);
    gdt64[idx].base_middle  = (base >> 16) & 0xFF;
    gdt64[idx].access       = access;
    gdt64[idx].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt64[idx].base_high    = (base >> 24) & 0xFF;
}

void init_gdt64() {
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Code: base=0, limit=0, Access=0x9A, Flags=0x20 (L-bit)
    gdt_set_entry(1, 0, 0, 0x9A, 0x20);

    // Data: base=0, limit=0, Access=0x92, Flags=0x00
    gdt_set_entry(2, 0, 0, 0x92, 0x00);

    gdt64_ptr.limit = sizeof(gdt64) - 1;
    gdt64_ptr.base  = (uint64_t)&gdt64;
}

#define ENTRIES_PER_TABLE 512
#define P_PRESENT (1ULL << 0)
#define P_WRITABLE (1ULL << 1)
#define P_USER (1ULL << 2)

typedef uint64_t pte_t;

struct pagetable {
    pte_t entries[ENTRIES_PER_TABLE];
};

struct pagetable* load_paging(void){
    //Allocate pagetables
    struct pagetable* pml4 = (void*)0x15000;
    struct pagetable* pdpt = (void*)0x16000;
    struct pagetable* pd = (void*)0x17000;
    struct pagetable* pt = (void*)0x18000;

    //Initialize tables to 0
    for (int i = 0; i < ENTRIES_PER_TABLE; i++){
        pml4->entries[i] = 0;
        pdpt->entries[i] = 0;
        pd->entries[i] = 0;
        pt->entries[i] = 0;
    }

    //Identity map first MiB
    for (int i = 0; i < 256; i++){
        pt->entries[i] = (i * 0x1000) | P_PRESENT | P_WRITABLE;
    }

    pd->entries[0] = ((uint64_t) pt) | P_PRESENT | P_WRITABLE;
    pdpt->entries[0] = ((uint64_t) pd) | P_PRESENT | P_WRITABLE;
    pml4->entries[0] = ((uint64_t) pdpt) | P_PRESENT | P_WRITABLE;

    return pml4;
}

__attribute__((noreturn)) void enter_long_mode(struct pagetable* pml4);

void __attribute__((noreturn)) __attribute__((section(".text.stage3"))) state3_main(void) {
    init_gdt64();
    struct pagetable* pt = load_paging();
    enter_long_mode(pt);

    while (1) {
        __asm__ __volatile__ (
            "hlt"
        );
    }
}