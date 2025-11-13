#include <stdint.h>
#include <stddef.h>

#define KERNEL_BASE 0x20000
#define KERNEL_RAM_IDENTITY_BASE 0xFFFF800000000000ULL
#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000ULL

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

void init_gdt64(void) {
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

struct bios_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

typedef uint64_t pte_t;

struct pagetable {
    pte_t entries[ENTRIES_PER_TABLE];
};

#define PD_BASE_INDEX 0
#define PDP_BASE_INDEX 510
#define PML4_BASE_INDEX 511

static inline struct pagetable* alloc_table(uintptr_t* next_addr, int* pt_count){
    struct pagetable *tbl = (struct pagetable*) *next_addr;
    *next_addr += 0x1000;
    (*pt_count)++;
    return tbl;
}

void load_paging(void) {
    int *pt_count = (int*)(uintptr_t)0x74FB;
    *pt_count = 0;

    uintptr_t pagetable_next_addr = 0x100000; /* PML4 start (physical) */

    struct pagetable *pml4  = alloc_table(&pagetable_next_addr, pt_count);
    struct pagetable *kpdpt = alloc_table(&pagetable_next_addr, pt_count);
    struct pagetable *kpd   = alloc_table(&pagetable_next_addr, pt_count);
    struct pagetable *kpt   = alloc_table(&pagetable_next_addr, pt_count);

    struct pagetable *ipdpt = alloc_table(&pagetable_next_addr, pt_count);
    struct pagetable *ipd   = alloc_table(&pagetable_next_addr, pt_count);
    struct pagetable *ipt   = alloc_table(&pagetable_next_addr, pt_count);

    /* zero tables */
    for (int i = 0; i < ENTRIES_PER_TABLE; ++i) {
        pml4->entries[i] = 0;
        kpdpt->entries[i] = 0;
        kpd->entries[i] = 0;
        kpt->entries[i] = 0;
        ipdpt->entries[i] = 0;
        ipd->entries[i] = 0;
        ipt->entries[i] = 0;
    }

    /* Identity-map first 2 MiB (1 PD -> 512 PT entries * 4KiB = 2MiB) */
    for (int i = 0; i < ENTRIES_PER_TABLE; ++i) {
        pte_t phys = (pte_t)((uintptr_t)i * 0x1000);
        ipt->entries[i] = phys | P_PRESENT | P_WRITABLE;
    }

    ipd->entries[0]   = ((pte_t)(uintptr_t)ipt & ~0xFFFULL) | P_PRESENT | P_WRITABLE;
    ipdpt->entries[0] = ((pte_t)(uintptr_t)ipd & ~0xFFFULL) | P_PRESENT | P_WRITABLE;
    pml4->entries[0]  = ((pte_t)(uintptr_t)ipdpt & ~0xFFFULL) | P_PRESENT | P_WRITABLE;

    /* Map kernel physical pages (KERNEL_BASE) into high-half virtual at
       PML4[511]->PDPT[256]->PD[0]->PT */
    for (int i = 0; i < KERNEL_PG_CNT; ++i) {
        pte_t phys = (pte_t)(KERNEL_BASE + (uintptr_t)i * 0x1000);
        kpt->entries[i] = (phys & ~0xFFFULL) | P_PRESENT | P_WRITABLE;
    }

    kpd->entries[PD_BASE_INDEX]   = ((pte_t)(uintptr_t)kpt & ~0xFFFULL) | P_PRESENT | P_WRITABLE;
    kpdpt->entries[PDP_BASE_INDEX]= ((pte_t)(uintptr_t)kpd & ~0xFFFULL) | P_PRESENT | P_WRITABLE;
    pml4->entries[PML4_BASE_INDEX]= ((pte_t)(uintptr_t)kpdpt & ~0xFFFULL) | P_PRESENT | P_WRITABLE;

    /* done — assembly expects the PML4 at 0x100000 (cr3 will be loaded with that) */
}

__attribute__((noreturn)) void enter_long_mode(void);

void __attribute__((noreturn)) __attribute__((section(".text.stage3"))) stage3_main(void) {
    init_gdt64();
    load_paging();
    enter_long_mode();

    while (1) {
        __asm__ __volatile__ (
            "hlt"
        );
    }
}