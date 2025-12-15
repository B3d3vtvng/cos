#include "vga.h"
#include <stdint.h>

static char* vga_text_buf_virt = (char*)VGA_START;
static char* vga_text_buf_pos  = (char*)VGA_START;
static char* vga_cursor_pos    = (char*)VGA_START;

/*
    * Remap the VGA text buffer to a different address
    * Doesn't work as of now :(
*/
void vga_remap_buffer(char* new_virt_base)
{
    uint64_t pos_offset  = vga_text_buf_pos - vga_text_buf_virt;
    uint64_t curs_offset = vga_cursor_pos   - vga_text_buf_virt;

    vga_text_buf_virt = new_virt_base;
    vga_text_buf_pos  = new_virt_base + pos_offset;
    vga_cursor_pos    = new_virt_base + curs_offset;

    for (size_t i = (size_t)vga_text_buf_virt; i < vga_text_buf_virt + VGA_BUFFER_SIZE; i++)
        *(char*)i = VGA_START[i - (uint64_t)vga_text_buf_virt];
}

// Output a byte to the specified port
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %0,%1" : : "a"(val), "Nd"(port));
}

// Hide the hardware cursor
void vga_cursor_hide(void)
{
    outb(CURSOR_CMD_PORT, 0x0A);
    outb(CURSOR_DATA_PORT, 0x20);
}

// Show the hardware cursor
void vga_cursor_show(void)
{
    outb(CURSOR_CMD_PORT, 0x0A);
    outb(CURSOR_DATA_PORT, 0x00);
    outb(CURSOR_CMD_PORT, 0x0B);
    outb(CURSOR_DATA_PORT, 0xFF);
}

// Set the hardware cursor position
static void set_hw_cursor(uint16_t pos)
{
    outb(CURSOR_CMD_PORT, 0x0F);
    outb(CURSOR_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(CURSOR_CMD_PORT, 0x0E);
    outb(CURSOR_DATA_PORT, (uint8_t)(pos >> 8));
}

// Scroll the VGA text buffer up by one line
static void vga_do_scroll(void)
{
    char* dst = vga_text_buf_virt;
    char* src = vga_text_buf_virt + 160;

    for (int i = 0; i < 24 * 160; i += 160)
        for (int j = 0; j < 160; j++)
            dst[i + j] = src[i + j];

    vga_text_buf_pos -= 160;
    for (int i = 0; i < 160; i++)
        vga_text_buf_pos[i] = (i & 1) ? 0x07 : ' ';
}

// Update the hardware cursor to match the current position
static void vga_update_cursor(void)
{
    uint16_t offset = (uint16_t)((vga_text_buf_pos - vga_text_buf_virt) >> 1);
    set_hw_cursor(offset);
}

// Output a character to the screen
void vga_putc(char c)
{
    if (c == '\n') {
        uint16_t col = (uint16_t)((vga_text_buf_pos - vga_text_buf_virt) % 160);
        vga_text_buf_pos += 160 - col;
    } else {
        *vga_text_buf_pos++ = c;
        *vga_text_buf_pos++ = 0x07;
    }

    if (vga_text_buf_pos >= vga_text_buf_virt + VGA_BUFFER_SIZE) {
        vga_do_scroll();
    }
    vga_update_cursor();
}

// Output a null-terminated string to the screen
void vga_print(const char* str)
{
    while (*str)
        vga_putc(*str++);
}

// Clear the screen
void vga_clear(void)
{
    for (char* p = vga_text_buf_virt; p < vga_text_buf_virt + VGA_BUFFER_SIZE; p += 2) {
        *p       = ' ';
        *(p + 1) = 0x07;
    }
    vga_text_buf_pos = vga_text_buf_virt;
    vga_cursor_pos   = vga_text_buf_virt;
    vga_update_cursor();
}