#include "vmm.h"

extern struct alloc_metadata alloc_meta; 
extern char _text_start, _rodata_start, _data_start, _bss_start, _kernel_end;

size_t get_kernel_pg_count(void){
    return (round_page_up((uint64_t)&_kernel_end - (uint64_t)&_text_start)) >> 12;
}

void set_section_prot(struct pagetable* pml4, uintptr_t sec_start, uintptr_t sec_end, uint64_t sec_flags) {
    // Ensure page-aligned
    if ((sec_start & 0xFFF) != 0 || (sec_end & 0xFFF) != 0 || sec_start >= sec_end) {
        kernel_panic(NULL, "Encountered non-aligned section address\n\n");
    }

    size_t sec_pg_count = (sec_end - sec_start) / 0x1000;
    
    for (size_t i = 0; i < sec_pg_count; i++) {
        set_pagetable_prot(pml4, sec_start + i * 0x1000, sec_flags, false);
    }

    return;
}

void make_ram_map(struct pagetable* pml4){
    uint64_t page_count = round_page_down(get_mem_max()) / 0x1000;
    uint64_t virt_cur = 0xFFFF800000000000ULL;
    uint64_t phys_cur = 0x0ULL;

    for (int i = 0; i < 0x200000; i += 0x1000){
        if (phys_cur == 0xB8000){
            //map_virtual(pml4, virt_cur, phys_cur, P_KERNEL | P_NX_ENABLE | P_PRESENT | P_WRITABLE, PG_SIZE_REG);
        } else {
            map_virtual(pml4, virt_cur, phys_cur, P_KERNEL | P_NX_ENABLE | P_PRESENT | P_WRITABLE, PG_SIZE_REG);
        }
        virt_cur += 0x1000;
        phys_cur += 0x1000;
        page_count--;
    }

    while (page_count > 512){
        map_virtual(pml4, virt_cur, phys_cur, P_KERNEL | P_NX_ENABLE | P_PRESENT | P_WRITABLE, PG_SIZE_LARGE);
        virt_cur += 0x200000;
        phys_cur += 0x200000;
        page_count -= 512;
    }

    if (page_count == 0) return;

    while(page_count != 0){
        map_virtual(pml4, virt_cur, phys_cur, P_KERNEL | P_NX_ENABLE | P_PRESENT | P_WRITABLE, PG_SIZE_REG);
        virt_cur += 0x1000;
        phys_cur += 0x1000;
        page_count--;
    }
}

extern void update_stack_ptr(uint64_t new_rsp, uint64_t new_rbp);

uint64_t update_stack_mappings(void){
    struct pagetable* pml4 = get_pgtable();
    uint64_t stack_phys_base = 0x10000ULL;
    uint64_t stack_phys_top = 0x19000ULL;
    uint64_t stack_virt_base = (uint64_t)alloc_virt((stack_phys_top - stack_phys_base) >> 12);
    // Map a guard page below the stack
    map_virtual(pml4, stack_virt_base, 0x0, P_NX_ENABLE | P_KERNEL, PG_SIZE_REG);
    for (uint64_t addr = 0x1000; addr < (stack_phys_top - stack_phys_base); addr += 0x1000){
        map_virtual(pml4, stack_virt_base + addr, stack_phys_base + addr, P_PRESENT | P_WRITABLE | P_KERNEL | P_NX_ENABLE, PG_SIZE_REG);
    }

    // Update the stack pointer
    uint64_t rbp, rsp;
    __asm__ __volatile__ ("mov %%rbp, %0" : "=r"(rbp));
    __asm__ __volatile__ ("mov %%rsp, %0" : "=r"(rsp));

    update_stack_ptr(stack_virt_base + (rsp - stack_phys_base), stack_virt_base + (rbp - stack_phys_base));

    vga_print("Updated stack mappings!\n");
    return stack_virt_base + (stack_phys_top - stack_phys_base);
}

void _update_pagetable_mappings(struct pagetable* pgtable, int depth){
    if (depth == 0) return;

    for (int i = 0; i < ENTRIES_PER_TABLE; i++){
        pte_t entry = pgtable->entries[i];
        if (entry & P_PRESENT && entry < 0xffff800000000000ULL){
            uint64_t phys_addr = entry & ~0xFFFULL;
            uint64_t virt_addr = phys_addr + 0xFFFF800000000000ULL;

            map_virtual(get_pgtable(), virt_addr, phys_addr, entry & 0xFFFULL, (entry & P_PS) ? PG_SIZE_LARGE : PG_SIZE_REG);

            if (!(entry & P_PS)){
                struct pagetable* next_level = (struct pagetable*)(uintptr_t)(phys_addr);
                _update_pagetable_mappings(next_level, depth - 1);
            }
        }
    }
}

void update_pagetable_mappings(void){
    struct pagetable* pml4 = get_pgtable();
    _update_pagetable_mappings(pml4, 3);

    map_virtual(pml4, (uint64_t)pml4 + 0xFFFF800000000000ULL, (uint64_t)pml4, P_PRESENT | P_WRITABLE | P_KERNEL, PG_SIZE_REG);
    invtlb();

    set_pgtable(pml4 + 0xFFFF800000000000ULL);

    vga_print("Updated pagetable mappings!\n");
}

void unmap_idmap(void){
    struct pagetable* pml4 = get_pgtable();
    for (uint64_t addr = 0x0; addr < 0x200000; addr += 0x1000){
        unmap_virtual(pml4, addr, PG_SIZE_REG);
    }
    invtlb();
    vga_print("Unmapped identity mapping!\n");
}

void update_gdt_and_tss(struct gdt_ptr* gdt_ptr, uint64_t stack_virt_top){
    struct tss_desc* tss_descriptor = (struct tss_desc*)((uint64_t)gdt_ptr->base + 5 * sizeof(struct gdt_entry));
    tss_descriptor->rsp1 = stack_virt_top;
    tss_descriptor->ist1 += 0xffff800000000000ULL;

    __asm__ __volatile__ ("lgdt %0" : : "m" (*gdt_ptr));
    return;
}

void switch_virt(struct gdt_ptr* gdt_ptr){
    char* vga_text_buf_virt_ = alloc_virt(1); // Allocate a virtual page and then map them ourselves
    map_virtual(get_pgtable(), (uint64_t)vga_text_buf_virt_, (uint64_t)VGA_START, P_PRESENT | P_WRITABLE | P_KERNEL | P_PCD | P_PWT, PG_SIZE_REG);
    vga_remap_buffer(vga_text_buf_virt_);

    unmap_virtual(get_pgtable(), (uint64_t)VGA_START, PG_SIZE_REG);
    invlpg((uint64_t)vga_text_buf_virt_);

    vga_print("Remapped vga text buffer!\n");

    uint64_t kern_stack_addr = update_stack_mappings();
    update_pagetable_mappings();
    update_gdt_and_tss(gdt_ptr, kern_stack_addr);

    set_paging_allocator(vpgalloc);
    set_paging_free(vpgfree);

    unmap_idmap();
    return;
}


void vmm_init(struct gdt_ptr* gdt_ptr){
    struct pagetable* pml4 = get_pgtable();

    set_section_prot(pml4, (uintptr_t)&_text_start, (uintptr_t)&_rodata_start, P_PRESENT | P_READONLY | P_KERNEL);
    set_section_prot(pml4, (uintptr_t)&_rodata_start, (uintptr_t)&_data_start, P_PRESENT | P_READONLY | P_KERNEL | P_NX_ENABLE);
    set_section_prot(pml4, (uintptr_t)&_data_start, (uintptr_t)&_bss_start, P_PRESENT | P_WRITABLE | P_KERNEL | P_NX_ENABLE);
    set_section_prot(pml4, (uintptr_t)&_bss_start, (uintptr_t)&_kernel_end, P_PRESENT | P_WRITABLE | P_KERNEL | P_NX_ENABLE);

    make_ram_map(pml4);
    invtlb();

    liballoc_set_pgalloc(vpgalloc);
    liballoc_set_pgfree(vpgfree);

    vmalloc_init();

    switch_virt(gdt_ptr);
}

