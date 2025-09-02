#ifndef __VGA_CONTROL_H__
#define __VGA_CONTROL_H__

#define VGA_MAX ((char*)0xB8FA0)
#define VGA_START ((char*)0xB8000)
#define CURSOR_CMD_PORT 0x3D4
#define CURSOR_DATA_PORT 0x3D5

void vga_print(const char* str);
void vga_clear(void);


#endif