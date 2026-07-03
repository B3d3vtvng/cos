#ifndef LAPIC_H
#define LAPIC_H

#include <stdbool.h>

#include "../../mem/vmalloc.h"
#include "../../util/printf.h"
#include "../../util/asm.h"
#include "../../idt/idt.h"
#include "../../util/string.h"

#define LAPIC_ID_OFF 0x20
#define LAPIC_VER_OFF 0x30
#define LAPIC_TPR_OFF 0x80
#define LAPIC_APR_OFF 0x90
#define LAPIC_PPR_OFF 0xA0
#define LAPIC_EOI_REG_OFF 0xB0
#define LAPIC_RRD_OFF 0xC0
#define LAPIC_LDR_OFF 0xD0
#define LAPIC_DFR_OFF 0xE0
#define LAPIC_SIVR_OFF 0xF0
#define LAPIC_ISR_OFF 0x100
#define LAPIC_TMR_OFF 0x180
#define LAPIC_IRR_OFF 0x200
#define LAPIC_ESR_OFF 0x280
#define LAPIC_LVT_CMCIR_OFF 0x2F0
#define LAPIC_ICR_OFF 0x300
#define LAPIC_LVT_TIMER_REG_OFF 0x320
#define LAPIC_LVT_TSR_OFF 0x330
#define LAPIC_LVT_PMCR_OFF 0x340
#define LAPIC_LVT_LINT0_REG_OFF 0x350
#define LAPIC_LVT_LINT1_REG_OFF 0x360
#define LAPIC_LVT_ER_REG_OFF 0x370
#define LAPIC_TIMER_ICR_OFF 0x380
#define LAPIC_TIMER_CCR_OFF 0x390
#define LAPIC_TIMER_DCR_OFF 0x3E0

#define LAPIC_SIVR_SOFTWARE_ENABLE 0x100


void init_apic(struct idt_ptr idt_ptr);
void lapic_write(uint32_t reg, uint32_t val);
uint32_t lapic_read(uint32_t reg);

#endif