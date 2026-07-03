section .text
    extern isr_apic_err_handler

global isr_siv
isr_siv:
    iretq

global isr_apic_err
isr_apic_err:
    call isr_apic_err_handler
    iretq