__attribute__((noreturn)) 
__attribute__((section(".text.entry")))
void kmain(void) {
    volatile char* video_memory = (volatile char*)0xB8010;
    const char* msg = "Kernel loaded, initializing neccessary components...";

    for (int i = 0; i < 80 * 25 * 2; i++) {
        video_memory[i] = 0; // Clear screen
    }
    
    for (int i = 0, j = 0; msg[i] != '\0'; i++, j+=2) {
        video_memory[j] = msg[i];       // Character
        video_memory[j + 1] = 0x07;     // Attribute byte (light grey on black)
    }
    
    // Halt forever
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}