ENTRY(_start)
SECTIONS
{
    . = 0x2000;
    .text : {
        *(.text.stage3)
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
