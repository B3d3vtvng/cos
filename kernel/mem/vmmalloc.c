#include "vmmalloc.h"

extern struct alloc_metadata alloc_meta; 
extern char _text_start, _rodata_start, _data_start, _bss_start, _kernel_end;

struct pagetable* get_pgtable(void){
    struct pagetable* pgtable;
    __asm__ __volatile__ ("mov %%cr3, %0" : "=r" (pgtable));

    return pgtable;
}

static inline uint64_t* get_pg_entry(uint64_t addr){
    
}

void pmm_init(void){
    
}

