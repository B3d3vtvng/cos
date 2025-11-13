#include "vga.h"

char* vga_text_buf_pos = (char*)VGA_START;
char* vga_cursor_pos = (char*)VGA_START;

static inline __attribute__((always_inline)) void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ (
        "outb %0, %1"
        : 
        : "a"(val), "Nd"(port)
    );
}

void vga_cursor_hide(void) {
    outb(CURSOR_CMD_PORT, 0x0A);
    outb(CURSOR_DATA_PORT, 0x20);
}

void vga_cursor_show(void) {
    outb(CURSOR_CMD_PORT, 0x0A);
    outb(CURSOR_DATA_PORT, 0x0F); // Set low byte of cursor start
    outb(CURSOR_CMD_PORT, 0x0B);
    outb(CURSOR_DATA_PORT, 0x0E); // Set high byte of cursor end
}

void set_cursor_pos(int row, int col){
    unsigned short pos = (row * 80) + col;
    outb(CURSOR_CMD_PORT, 0x0F);
    outb(CURSOR_DATA_PORT, (unsigned char)(pos & 0xFF));
    outb(CURSOR_CMD_PORT, 0x0E);
    outb(CURSOR_DATA_PORT, (unsigned char)((pos >> 8) & 0xFF));
}

static void vga_do_scroll(void) {
    for (char* dest = (char*)VGA_START, *src = (char*)VGA_START + 160; src < (char*)VGA_MAX; dest += 160, src += 160) {
        for (int i = 0; i < 160; i++) {
            dest[i] = src[i];
        }
    }
    vga_text_buf_pos -= 160;
    for (int i = 0; i < 160; i++) {
        vga_text_buf_pos[i] = (i % 2 == 0) ? ' ' : 0x07;
    }
}

static void vga_update_cursor(void) {
    int offset = (int)(vga_text_buf_pos - (char*)VGA_START);
    int row = offset / 160;
    int col = (offset % 160) / 2;
    set_cursor_pos(row, col);
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_text_buf_pos += 160 - ((vga_text_buf_pos - (char*)VGA_START) % 160);
    } else {
        *vga_text_buf_pos++ = c;
        *vga_text_buf_pos++ = 0x07; // Light grey on black
    }

    if (vga_text_buf_pos >= (char*)VGA_MAX) {
        vga_do_scroll();
    }

    vga_update_cursor();
}

void vga_print(const char* str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_clear(void) {
    for (char* p = (char*)VGA_START; p < (char*)VGA_MAX; p += 2) {
        *p = ' ';
        *(p + 1) = 0x07; // Light grey on black
    }
    vga_text_buf_pos = (char*)VGA_START;
    vga_update_cursor();
}