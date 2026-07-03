#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "../../util/printf.h"
#include "../apic/apic.h"
#include "../../sched/proc.h"
#include "../../idt/idt.h"

#define TIMER_LVT_ONE_SHOT (0ULL << 17)
#define TIMER_LVT_PERIODIC (1ULL << 17)
#define TIMER_LVT_TSC_DEADSHOT (2ULL << 17)
#define TIMER_LVT_INT_UNMASK (0ULL << 16)
#define TIMER_LVT_INT_MASK (1ULL << 16)

struct notify_queue{
    void (*notify_callback)(struct proc_state*);
    uint32_t id;
    uint32_t ticks;
    uint32_t ticks_cur;
    bool once;
    struct notify_queue* next;
};

void timer_init(struct idt_ptr idtp);
uint32_t notify_add(uint64_t ms, void (*notify_callback)(struct proc_state*));
uint32_t notify_once(uint64_t ms, void (*notify_callback)(struct proc_state*));
void notify_remove(uint32_t id);

#endif