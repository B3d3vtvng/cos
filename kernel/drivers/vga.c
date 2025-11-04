#include "vga.h"

char* vga_text_buf_pos = (char*)0xB8000;
char* vga_cursor_pos = (char*)0xB8000;

static inline __attribute__((always_inline)) void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ (
        "outb %0, %1"
        : 
        : "a"(val), "Nd"(port)
    );
}

void vga_cursor_hide(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_cursor_show(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0F); // Set low byte of cursor start
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0E); // Set high byte of cursor end
}

void set_cursor_pos(int row, int col){
    unsigned short pos = (row * 80) + col;
    outb(CURSOR_CMD_PORT, 0x0F);
    outb(CURSOR_DATA_PORT, (unsigned char)(pos & 0xFF));
    outb(CURSOR_CMD_PORT, 0x0E);
    outb(CURSOR_DATA_PORT, (unsigned char)((pos >> 8) & 0xFF));
}

void vga_print(const char* str) {
    if (vga_text_buf_pos >= VGA_MAX) {
        // Scroll up
        for (char* dest = VGA_START, *src = VGA_START + 160; src < VGA_MAX; dest += 160, src += 160) {
            for (int i = 0; i < 160; i++) {
                dest[i] = src[i];
            }
        }
        vga_text_buf_pos -= 160;
        // Clear the last line
        for (int i = 0; i < 160; i++) {
            vga_text_buf_pos[i] = (i % 2 == 0) ? ' ' : 0x07;
        }

        // Move cursor to the end
        set_cursor_pos((vga_text_buf_pos - VGA_START) / 160, ((vga_text_buf_pos - VGA_START) % 160) / 2);
    }

    while (*str) {
        if (*str == '\n') {
            vga_text_buf_pos += 160 - ((vga_text_buf_pos - VGA_START) % 160);
        } else {
            *vga_text_buf_pos++ = *str;
            *vga_text_buf_pos++ = 0x07; // Light grey on black
        }
        str++;
        if (vga_text_buf_pos >= VGA_MAX) {
            // Scroll up
            for (char* dest = VGA_START, *src = VGA_START + 160; src < VGA_MAX; dest += 160, src += 160) {
                for (int i = 0; i < 160; i++) {
                    dest[i] = src[i];
                }
            }
            vga_text_buf_pos -= 160;
            // Clear the last line
            for (int i = 0; i < 160; i++) {
                vga_text_buf_pos[i] = (i % 2 == 0) ? ' ' : 0x07;
            }
        }
    }

    // Update cursor position
    set_cursor_pos((vga_text_buf_pos - VGA_START) / 160, ((vga_text_buf_pos - VGA_START) % 160) / 2);
}

void vga_clear(void) {
    for (char* p = VGA_START; p < VGA_MAX; p += 2) {
        *p = ' ';
        *(p + 1) = 0x07; // Light grey on black
    }
    vga_text_buf_pos = VGA_START;
}