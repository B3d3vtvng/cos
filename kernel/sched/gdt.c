#include "gdt.h"

uint64_t get_tss_base(struct tss_desc* desc) {
    uint64_t base = 0;

    base |= (uint64_t)(desc->base0);                   // bits  0-15
    base |= (uint64_t)(desc->base1) << 16;             // bits 16-23
    base |= (uint64_t)(desc->base2) << 24;             // bits 24-31
    base |= (uint64_t)(desc->base3) << 32;             // bits 32-63

    return base;
}

void set_tss_base(struct tss_desc* tss_descriptor, uint64_t base){
    tss_descriptor->base0 = base & 0xFFFF;
    tss_descriptor->base1 = (base >> 16) & 0xFF;
    tss_descriptor->base2 = (base >> 24) & 0xFF;
    tss_descriptor->base3 = (base >> 32) & 0xFFFFFFFF;
}

void gdt_set_entry(struct gdt_entry* entry, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t gran) {
    entry->limit_low    = (limit & 0xFFFF);
    entry->base_low     = (base & 0xFFFF);
    entry->base_middle  = (base >> 16) & 0xFF;
    entry->access       = access;
    entry->granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entry->base_high    = (base >> 24) & 0xFF;
}

void init_tss(struct tss* tss, void* kern_stack, void* excp_df_stack){
    for (int i = 0; i < sizeof(struct tss); i++){
        ((char*) tss)[i] = 0;
    }

    tss->ist1 = (uint64_t)excp_df_stack + 0xFF0;
    tss->io_map_base = 0xFFFF;
    tss->rsp1 = (uint64_t)kern_stack;
}

struct gdt_ptr init_gdt_and_tss(void){
    struct gdt_ptr gdtp;
    struct gdt_entry* entries = kmalloc(sizeof(struct gdt_entry) * 7);
    struct tss_desc* tss_descriptor = (struct tss_desc*) &entries[5];
    uint64_t tss_phys = (uint64_t)kmalloc(sizeof(struct tss));

    // Initialize tss descriptor
    tss_descriptor->base0 = tss_phys & 0xFFFF;
    tss_descriptor->base1 = (tss_phys >> 16) & 0xFF;
    tss_descriptor->base2 = (tss_phys >> 24) & 0xFF;
    tss_descriptor->base3 = (tss_phys >> 32) & 0xFFFFFFFF;
    uint16_t tss_limit = sizeof(struct tss) - 1;

    tss_descriptor->limit0 = tss_limit & 0xFFFF;
    tss_descriptor->limit1_flags = (tss_limit >> 16) & 0x0F | 0x00;

    tss_descriptor->reserved = 0;
    tss_descriptor->access = 0x89;

    void* kern_stack = (void*)0x19000;
    void* excp_df_stack = pmmalloc(1); // Allocate one page as a stack for double faults (might change later)

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

    init_tss((struct tss*)tss_phys, kern_stack, excp_df_stack);

    gdtp.base = (uint64_t)entries;
    gdtp.limit = (sizeof(struct gdt_entry) * 7) - 1;

    lgdt(&gdtp);

    return gdtp;
}