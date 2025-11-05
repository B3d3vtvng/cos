#include "pmmalloc.h"

static struct pmmpg_state pmm_state;

void pmm_init(void){
    int* bios_mm_size = (int*)BIOS_MM_SIZE_ADDR;
    struct bios_mm_entry* bios_mm = BIOS_MM_ADDR;

    for (int i = 0; i < *bios_mm_size / sizeof(struct bios_mm_entry); i++){
        if (bios_mm[i].type == 1){ // Usable RAM
            size_t start_page = bios_mm[i].base_addr / PAGE_SIZE;
            size_t length_pages = bios_mm[i].length / PAGE_SIZE;

            for (size_t j = 0; j < length_pages; j++){
                size_t page_index = start_page + j;
                pmm_state.free_page_bitmap[page_index / 8] |= (1 << (page_index % 8));
            }
        }
    }
}