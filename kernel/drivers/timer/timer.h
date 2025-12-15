#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>

#include "../../mem/vmalloc.h"
#include "../../util/printf.h"
#include "../../util/asm.h"

void init_timer_driver(void);
void sleep_ms(uint64_t ms);
void notify_add(uint32_t id, uint64_t ms);
void notify_remove(uint32_t id);

#endif