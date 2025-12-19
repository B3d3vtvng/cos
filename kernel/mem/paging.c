#include "paging.h"

static bool _paging_access_direct = true;

void set_paging_access(bool direct){
    _paging_access_direct = direct;
}

// Checks whether a page table entry is present
static inline int is_present(uint64_t entry) {
    return (entry & P_PRESENT);
}

// Rounds down an address to the nearest page boundary, used for page table entries
static inline uintptr_t get_next_table_addr(uint64_t entry) {
    return (entry & 0x000FFFFFFFFFF000ULL);
}

// Invalidates the entire Translation Lookaside Buffer (TLB)
void invtlb(void){
    uint64_t cr3; 
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

// Returns the page fault linear address from CR2 register
uint64_t get_cr2(void){
    uint64_t cr2;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

// Invalidates a single page in the TLB
void invlpg(uint64_t addr) {
    __asm__ __volatile__ (
        "invlpg (%0)" :: "r" (addr) : "memory"
    );
}

/*
    * Retrieve the page table entry for a given virtual address
    * Returns NULL if the entry is not present
*/
uint64_t* get_pg_entry(struct pagetable* pml4, uint64_t addr){
    struct pagetable* cur_pgtable = pml4;
    if (!_paging_access_direct){
        pml4 += RAM_MAP_OFF;
    }
    uint64_t entry_value;

    entry_value = cur_pgtable->entries[(addr >> 39) & 0x1FF];
    if (!is_present(entry_value)) return NULL;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(addr >> 30) & 0x1FF];
    if (!is_present(entry_value)) return NULL;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(addr >> 21) & 0x1FF];
    if (!is_present(entry_value)) return NULL;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(addr >> 12) & 0x1FF];
    if (!is_present(entry_value)) return NULL;

   return (uint64_t*)&cur_pgtable->entries[(addr >> 12) & 0x1FF];
}

// Returns the currently active PML4
struct pagetable* get_pgtable(void){
    struct pagetable* pgtable;
    __asm__ __volatile__ ("mov %%cr3, %0" : "=r" (pgtable));

    return pgtable;
}

void set_pgtable(struct pagetable* pgtable){
    __asm__ __volatile__ ("mov %0, %%cr3" : : "r" (pgtable) : "memory");
}

// Sets a page table entry to a specific value
int set_pagetable_entry(struct pagetable* pml4, uint64_t vaddr, uint64_t val, int flush){
    uint64_t* entry = get_pg_entry(pml4, vaddr);

    if (entry == NULL){
        return 1;
    }

    *entry = val;

    if (flush){
        invlpg(vaddr);
    }

    return 0;
}

// Sets specific flags in a page table entry
int set_pagetable_prot(struct pagetable* pml4, uint64_t vaddr, uint64_t flags, int flush){
    uint64_t* entry = get_pg_entry(pml4, vaddr);

    if (entry == NULL){
        return 1;
    }

    *entry |= flags;

    if (flush){
        invlpg(vaddr);
    }

    return 0;
}

// Translates a virtual address to a physical address
// Returns 0 if the virtual address is not mapped
uint64_t virt_to_phys(uint64_t vaddr){
    uint64_t* entry = get_pg_entry(get_pgtable(), vaddr);

    if (entry == NULL) return 0;

    return round_page_down(*entry) + (vaddr - round_page_down(vaddr));
}

int map_virtual_pg_reg(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
int map_virtual_pg_large(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
int map_virtual_pg_huge(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags);

// Unmaps a virtual address from the page tables
void unmap_virtual(struct pagetable* pml4, uint64_t virt, enum pg_size pg_size){
    struct pagetable* cur_pgtable = pml4;
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        pml4 = (struct pagetable*)((uintptr_t)pml4 | (uintptr_t)RAM_MAP_OFF);
    }

    struct pagetable* pdpt, *pd, *pt;
    uint64_t entry_value;

    entry_value = cur_pgtable->entries[(virt >> 39) & 0x1FF];
    if (!is_present(entry_value)) return;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    pdpt = cur_pgtable;

    entry_value = cur_pgtable->entries[(virt >> 30) & 0x1FF];
    if (!is_present(entry_value)) return;
    if (pg_size == PG_SIZE_HUGE) goto pdpt_clear;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    pd = cur_pgtable;

    entry_value = cur_pgtable->entries[(virt >> 21) & 0x1FF];
    if (!is_present(entry_value)) return;
    if (pg_size == PG_SIZE_LARGE) goto pd_clear;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    pt = cur_pgtable;

    entry_value = cur_pgtable->entries[(virt >> 12) & 0x1FF];
    if (!is_present(entry_value)) return;

    cur_pgtable->entries[(virt >> 12) & 0x1FF] = 0;
    invlpg(virt);

    for (int i = 0; i < ENTRIES_PER_TABLE; i++){
        if (pt->entries[i] != 0) return;
    }

    pd_clear:
        pmmfree((void*)((uintptr_t)pt - RAM_MAP_OFF));
        
        pd->entries[(virt >> 21) & 0x1FF] = 0;
        for (int i = 0; i < ENTRIES_PER_TABLE; i++){
            if (pd->entries[i] != 0) return;
        }

    pdpt_clear:
        pmmfree((void*)((uintptr_t)pd - RAM_MAP_OFF));

        pdpt->entries[(virt >> 30) & 0x1FF] = 0;
        for (int i = 0; i < ENTRIES_PER_TABLE; i++){
            if (pdpt->entries[i] != 0) return;
        }

    pmmfree((void*)((uintptr_t)pdpt - RAM_MAP_OFF));
    pml4->entries[(virt >> 39) & 0x1FF] = 0;
}

// Maps a virtual address to a physical address with specified page size and flags
int map_virtual(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags, enum pg_size pg_size){
    if (pg_size == PG_SIZE_REG){
        return map_virtual_pg_reg(pml4, virt, phys, flags);
    }

    if (pg_size == PG_SIZE_LARGE){
        return map_virtual_pg_large(pml4, virt, phys, flags);
    }

    return map_virtual_pg_huge(pml4, virt, phys, flags);
}

int map_virtual_pg_reg(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags){
    if (phys & ((1ULL << 12) - 1)){
        return 1;
    }

    struct pagetable* cur_pgtable = pml4;
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    uint64_t entry_value;

    entry_value = cur_pgtable->entries[(virt >> 39) & 0x1FF];
    if (!is_present(entry_value)) goto pdpt_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(virt >> 30) & 0x1FF];
    if (!is_present(entry_value)) goto pd_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(virt >> 21) & 0x1FF];
    if (!is_present(entry_value)) goto pt_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    goto set_entry;

    pdpt_new:
        struct pagetable* new_pdpt = pmmalloc(1);
        pml4->entries[(virt >> 39) & 0x1FF] = (uint64_t)new_pdpt | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pdpt;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }

    pd_new:
        struct pagetable* new_pd = pmmalloc(1);
        cur_pgtable->entries[(virt >> 30) & 0x1FF] = (uint64_t)new_pd | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pd;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }

    pt_new:
        struct pagetable* new_pt = pmmalloc(1);
        cur_pgtable->entries[(virt >> 21) & 0x1FF] = (uint64_t)new_pt | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pt;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }
    
    set_entry:
        cur_pgtable->entries[(virt >> 12) & 0x1FF] = phys | flags | P_PRESENT;

    return 0;
}

int map_virtual_pg_large(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags){
    if (phys & ((1ULL << 21) - 1)){
        return 1;
    }

    struct pagetable* cur_pgtable = pml4;
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    uint64_t entry_value;

    entry_value = cur_pgtable->entries[(virt >> 39) & 0x1FF];
    if (!is_present(entry_value)) goto pdpt_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    entry_value = cur_pgtable->entries[(virt >> 30) & 0x1FF];
    if (!is_present(entry_value)) goto pd_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    goto set_entry;

    pdpt_new:
        struct pagetable* new_pdpt = pmmalloc(1);
        pml4->entries[(virt >> 39) & 0x1FF] = (uint64_t)new_pdpt | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pdpt;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }

    pd_new:
        struct pagetable* new_pd = pmmalloc(1);
        cur_pgtable->entries[(virt >> 30) & 0x1FF] = (uint64_t)new_pd | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pd;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }
    
    set_entry:
        cur_pgtable->entries[(virt >> 21) & 0x1FF] = phys | flags | P_PS | P_PRESENT;

    return 0;
}

int map_virtual_pg_huge(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags){
    if (phys & ((1ULL << 30) - 1)){
        return 1;
    }

    struct pagetable* cur_pgtable = pml4;
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }
    uint64_t entry_value;

    entry_value = cur_pgtable->entries[(virt >> 39) & 0x1FF];
    if (!is_present(entry_value)) goto pdpt_new;
    cur_pgtable = (struct pagetable*)get_next_table_addr(entry_value);
    if (!_paging_access_direct){
        cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
    }

    goto set_entry;

    pdpt_new:
        struct pagetable* new_pdpt = pmmalloc(1);
        pml4->entries[(virt >> 39) & 0x1FF] = (uint64_t)new_pdpt | P_PRESENT | P_WRITABLE;
        cur_pgtable = new_pdpt;
        if (!_paging_access_direct){
            cur_pgtable = (struct pagetable*)((uintptr_t)cur_pgtable | (uintptr_t)RAM_MAP_OFF);
        }
    
    set_entry:
        cur_pgtable->entries[(virt >> 30) & 0x1FF] = phys | flags | P_PS | P_PRESENT;

    return 0;
}