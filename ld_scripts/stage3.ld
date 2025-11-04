ENTRY(stage3_main)
SECTIONS
{
    . = 0x15000;
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

