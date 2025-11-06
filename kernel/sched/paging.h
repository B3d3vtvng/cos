#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define ENTRIES_PER_TABLE 512
#define P_PRESENT (1ULL << 0)
#define P_WRITABLE (1ULL << 1)
#define P_USER (1ULL << 2)

typedef uint64_t pte_t;

struct pagetable {
    pte_t entries[ENTRIES_PER_TABLE];
};

#endif