#ifndef DISK_H
#define DISK_H

#include "../../mem/vmm.h"
#include "../../util/printf.h"

int read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer);
int write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer);


#endif