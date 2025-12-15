#ifndef VGA_H
#define VGA_H

#include <stddef.h>

#define VGA_MAX ((char*)0xB8FA0)
#define VGA_START ((char*)0xB8000)
#define VGA_WIDTH        80
#define VGA_HEIGHT       25
#define VGA_BUFFER_SIZE  (VGA_WIDTH * VGA_HEIGHT * 2)
#define CURSOR_CMD_PORT 0x3D4
#define CURSOR_DATA_PORT 0x3D5

void vga_remap_buffer(char* new_virt_base);

void vga_putc(char);
void vga_print(const char*);
void vga_clear(void);

#endif