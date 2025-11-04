ENTRY(kernel_main)
SECTIONS
{
    . = 0x20000;
    .text : {
        *(.text.kern_entry)
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
}
