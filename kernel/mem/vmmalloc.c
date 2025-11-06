#include "vmmalloc.h"

extern struct alloc_metadata alloc_meta; 

struct pagetable* vmm_init(void){
    struct pagetable* master_pml4 = kmalloc(sizeof(struct pagetable));

}