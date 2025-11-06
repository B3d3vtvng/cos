#include "gdt.h"

void gdt_set_entry(struct gdt_entry* entry, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t gran) {
    entry->limit_low    = (limit & 0xFFFF);
    entry->base_low     = (base & 0xFFFF);
    entry->base_middle  = (base >> 16) & 0xFF;
    entry->access       = access;
    entry->granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entry->base_high    = (base >> 24) & 0xFF;
}

void init_tss(struct tss_desc* tss_descriptor, void* kern_stack, void* excp_df_stack){
    for (int i = 0; i < sizeof(struct tss_desc); i++){
        ((char*) tss_descriptor)[i] = 0;
    }

    tss_descriptor->ist1 = (uint64_t)excp_df_stack;
    tss_descriptor->io_map_base = 128;
    tss_descriptor->rsp1 = (uint64_t)kern_stack;
}

void init_gdt_and_tss(void){
    struct gdt_ptr gdtp;
    struct gdt_entry* entries = kmalloc(sizeof(struct gdt_entry) * 7);

    // NULL
    gdt_set_entry(&entries[0], 0, 0, 0, 0);

    // Kernel Code
    gdt_set_entry(&entries[1], 0, 0, GDT_ACCESS_KERN_CODE, GDT_GRANULARITY_CODE);

    // Kernel Data
    gdt_set_entry(&entries[2], 0, 0, GDT_ACCESS_KERN_DATA, GDT_GRANULARITY_DATA);

    // User Code
    gdt_set_entry(&entries[3], 0, 0, GDT_ACCESS_USER_CODE, GDT_GRANULARITY_CODE);

    // User Data
    gdt_set_entry(&entries[4], 0, 0, GDT_ACCESS_DATA, GDT_GRANULARITY_DATA);

    // TSS
    void* kern_stack = (void*)0x19000; // Kernel stack base set in enter_long_mode
    void* excp_df_stack = kmalloc(0x1000); // Allocate one page for double faults (might change later)

    init_tss((struct tss_desc*) &entries[5], kern_stack, excp_df_stack);

    gdtp.base = (uint64_t)entries;
    gdtp.limit = sizeof(struct gdt_entry) * 7;

    __asm__ __volatile__ ("lgdt %0" : : "m" (gdtp));
    return;
}