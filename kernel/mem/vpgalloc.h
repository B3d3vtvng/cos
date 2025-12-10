#include <stdint.h>
#include <stddef.h>

#include "pmmalloc.h"

void* vpgalloc(size_t pg_count);
void vpgfree(void* ptr);