#include "paging.h"

struct pagetable* init_paging(void){
    //Allocate pagetables
    struct pagetable* pml4 = kpgmalloc(1);
    struct pagetable* pdpt = kpgmalloc(1);
    struct pagetable* pd = kpgmalloc(1);
    struct pagetable* pt = kpgmalloc(1);

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

    //Set page table address in cr3
    __asm__ __volatile__ ("mov %0, %%cr3" :: "r"(pml4));

    //Enable 4 level paging
    __asm__ __volatile__ (
        "mov eax, cr4"
        "or eax, 1 >> 5"
        "mov cr4, eax"
    );
    
    return pml4;
}