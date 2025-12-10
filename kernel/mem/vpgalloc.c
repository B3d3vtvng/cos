#include "vpgalloc.h"

void* vpgalloc(size_t pg_count){
    return (void*) ((uintptr_t)pmmalloc(pg_count) + 0xFFFF800000000000);
}

void vpgfree(void* ptr){
    pmmfree((void*) ((uintptr_t) ptr - 0xFFFF800000000000));
}