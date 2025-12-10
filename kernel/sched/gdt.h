#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#include "../mem/kmalloc.h"

struct gdt_entry {
    uint16_t limit_low;     // Bits 0-15 of limit
    uint16_t base_low;      // Bits 0-15 of base
    uint8_t  base_middle;   // Bits 16-23 of base
    uint8_t  access;        // Access flags
    uint8_t  granularity;   // Flags + bits 16-19 of limit
    uint8_t  base_high;     // Bits 24-31 of base
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;          // 64-bit base for lgdt
} __attribute__((packed));

struct tss_desc{
    uint32_t reserved1;
    uint64_t rsp1; // Kernel stack address
    uint64_t rsp2; // Unused -> NULL
    uint64_t rsp3; // Unused -> NULL
    uint64_t reserved2;
    uint64_t ist1; // Double fault stack address
    uint64_t ist_unused[6]; // stacks for other exceptions, for now unused
    uint64_t reserved3; // Must be 0
    uint16_t reserved4; // Must be 0
    uint16_t io_map_base; // Unused, set to limit (128 bytes)
    uint64_t padding[3]; //Pad to 128 bytes
} __attribute__((packed)); 

// --- Bit 7: Present ---
#define GDT_ACCESS_PRESENT      (1 << 7)

// --- Bit 6-5: Descriptor Privilege Level (DPL) ---
#define GDT_ACCESS_DPL_KERN     (0 << 5)  // 00b (Ring 0)
#define GDT_ACCESS_DPL_USER     (3 << 5)  // 11b (Ring 3)

// --- Bit 4: Descriptor Type (S) ---
#define GDT_ACCESS_S_CODE_DATA  (1 << 4)  // Code or Data Segment
#define GDT_ACCESS_S_SYSTEM     (0 << 4)  // System Segment (e.g., TSS)

// --- Bit 3: Executable (E) ---
#define GDT_ACCESS_E_EXEC       (1 << 3)
#define GDT_ACCESS_E_NOEXEC     (0 << 3)

// --- Bit 2: Direction/Conforming (Ignored for 64-bit) ---
// Not typically defined separately, as the E bit dictates usage.

// --- Bit 1: Read/Write (R/W) ---
#define GDT_ACCESS_RW           (1 << 1)  // Writable for Data, Readable for Code

// --- Bit 0: Accessed (A) ---
#define GDT_ACCESS_ACCESSED     (1 << 0)
#define GDT_ACCESS_NOT_ACCESSED (0 << 0)

#define GDT_ACCESS_CODE (GDT_ACCESS_PRESENT | GDT_ACCESS_S_CODE_DATA | GDT_ACCESS_E_EXEC | GDT_ACCESS_RW)
#define GDT_ACCESS_DATA (GDT_ACCESS_PRESENT | GDT_ACCESS_S_CODE_DATA | GDT_ACCESS_E_NOEXEC | GDT_ACCESS_RW)

#define GDT_ACCESS_KERN_CODE (GDT_ACCESS_DPL_KERN | GDT_ACCESS_CODE)
#define GDT_ACCESS_KERN_DATA (GDT_ACCESS_DPL_KERN | GDT_ACCESS_DATA)
#define GDT_ACCESS_USER_CODE (GDT_ACCESS_DPL_USER | GDT_ACCESS_CODE)
#define GDT_ACCESS_USER_DATA (GDT_ACCESS_DPL_USER | GDT_ACCESS_DATA)

// --- GDT granularity flags ---
#define GDT_GRANULARITY_G (1 << 7) // Interpret Segment offset as 1 byte
#define GDT_GRANULARITY_DB (0 << 6) // Ignored for 64-bit
#define GDT_GRANULARITY_L (1 << 5) // Long mode flag (must be 1 for code)
#define GDT_GRANULARITY_NOL (0 << 5)
#define GDT_GRANULARITY_AVL (0 << 4) // Always 0

#define GDT_GRANULARITY_CODE (GDT_GRANULARITY_G | GDT_GRANULARITY_DB | GDT_GRANULARITY_L | GDT_GRANULARITY_AVL)
#define GDT_GRANULARITY_DATA (GDT_GRANULARITY_G | GDT_GRANULARITY_DB | GDT_GRANULARITY_NOL | GDT_GRANULARITY_AVL)


struct gdt_ptr init_gdt_and_tss(void);

#endif