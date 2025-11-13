PAGE_SIZE = 0x1000;

ENTRY(kernel_main);
SECTIONS
{
    . = 0xFFFFFFFF80000000;
    .text : {
        *(.text.kern_entry);
        *(.text);
    }

    . = ALIGN(PAGE_SIZE);
    .rodata : {
        *(.rodata);
    }

    . = ALIGN(PAGE_SIZE);
    .data : {
        *(.data);
    }

    . = ALIGN(PAGE_SIZE);
    .bss : {
        *(.bss);
    }
    _kernel_end = .;

    _text_start = ADDR(.text);
    _data_start = ADDR(.data);
    _bss_start = ADDR(.bss);
    _rodata_start = ADDR(.rodata);
}
