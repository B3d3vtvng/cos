#define VGA_BASE (char*)0xB8000
#define VGA_ATTR_GREY 0x07

int kmain(void) {
    const char* msg = "Hello, Kernel World!";
    char* video_memory = VGA_BASE;
    for (int i = 0; msg[i] != '\0'; i++) {
        video_memory[i * 2] = msg[i];       // Character
        video_memory[i * 2 + 1] = VGA_ATTR_GREY;
    }

    return 0;
}