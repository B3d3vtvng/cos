#include "timer.h"

static spinlock_t timer_lock = SPINLOCK_INIT;
static struct notify_queue* nqueue = NULL;
static uint32_t id_next = 0;

extern void isr_timer(void);

#define CALIB_SLEEP_PIT_TICKS 11932
#define LAPIC_TIMER_PERIODIC (1 << 17)
#define TARGET_HZ 100

uint32_t _notify_add(uint64_t ms, void (*notify_callback)(struct proc_state*), bool once){
    flags_t flags = spinlock_aquire(&timer_lock);

    struct notify_queue* nqueue_new = kmalloc(sizeof(struct notify_queue));
    nqueue_new->id = id_next++;
    nqueue_new->notify_callback = notify_callback;
    nqueue_new->once = once;
    nqueue_new->next = NULL;
    nqueue_new->ticks = (ms < 10) ? 1 : ms / 10;
    nqueue_new->ticks_cur = nqueue_new->ticks;

    if (nqueue == NULL) {
        nqueue = nqueue_new;
    } else {
        struct notify_queue* last = nqueue;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = nqueue_new;
    }

    spinlock_release(&timer_lock, flags);
    return nqueue_new->id;
}

uint32_t notify_add(uint64_t ms, void (*notify_callback)(struct proc_state*)){
    return _notify_add(ms, notify_callback, false);
}

uint32_t notify_once(uint64_t ms, void (*notify_callback)(struct proc_state*)){
    return _notify_add(ms, notify_callback, true);
}

void notify_remove(uint32_t id){
    flags_t flags = spinlock_aquire(&timer_lock);

    struct notify_queue* cur = nqueue;
    struct notify_queue* prev = NULL;

    while (cur != NULL) {
        if (cur->id == id) {
            if (prev == NULL) {
                nqueue = cur->next;
            } else {
                prev->next = cur->next;
            }
            kfree(cur);
            spinlock_release(&timer_lock, flags);
            return;
        }
        prev = cur;
        cur = cur->next;
    }

    spinlock_release(&timer_lock, flags);
}

void handle_timer(struct proc_state* pstate){
    flags_t flags = spinlock_aquire(&timer_lock);

    struct notify_queue* cur = nqueue;
    struct notify_queue* prev = NULL;

    while (cur != NULL) {
        cur->ticks_cur--;
        
        if (cur->ticks_cur == 0) {
            cur->notify_callback(pstate);

            if (cur->once) {
                struct notify_queue* to_remove = cur;
                if (prev == NULL) {
                    nqueue = cur->next;
                    cur = nqueue;
                } else {
                    prev->next = cur->next;
                    cur = prev->next;
                }
                kfree(to_remove);
                continue; 
            } else {
                cur->ticks_cur = cur->ticks;
            }
        }
        
        prev = cur;
        if (cur) cur = cur->next;
    }

    spinlock_release(&timer_lock, flags);
}

void pit_prepare_sleep(void){
    io_outb(0x43, 0x30);
    io_outb(0x40, (uint8_t) (CALIB_SLEEP_PIT_TICKS & 0xFF));
    io_outb(0x40, (uint8_t) ((CALIB_SLEEP_PIT_TICKS >> 8) & 0xFF));
}

void timer_init(struct idt_ptr idtp){
    idt_set_gate((struct idt_entry*) idtp.base, 0x20, (uint64_t) isr_timer, 0x8, 0x8E);

    lapic_write(LAPIC_TIMER_CCR_OFF, 0);
    lapic_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_ONE_SHOT | 0x20);
    lapic_write(LAPIC_TIMER_DCR_OFF, 0x3);
    lapic_write(LAPIC_TIMER_ICR_OFF, 0xFFFFFFFF);

    pit_prepare_sleep();

    while (1){
        io_outb(0x43, 0);
        uint8_t lo = io_inb(0x40);
        uint8_t hi = io_inb(0x40);

        uint16_t count = lo | (hi << 8);
        //kprintf("PIT count: %u, apic_val: %u\n", count, lapic_read(LAPIC_TIMER_CCR_OFF));
        if (count == 0 || count > CALIB_SLEEP_PIT_TICKS) {
            break;
        }
    }

    uint32_t current_apic_val = lapic_read(LAPIC_TIMER_CCR_OFF);
    uint32_t ticks_in_10ms = 0xFFFFFFFF - current_apic_val;

    lapic_write(LAPIC_TIMER_ICR_OFF, 0);

    uint32_t initial_count = (ticks_in_10ms * 100) / TARGET_HZ;

    lapic_write(LAPIC_LVT_TIMER_REG_OFF, 0x0);
    lapic_write(LAPIC_LVT_TIMER_REG_OFF, 0x20 | TIMER_LVT_PERIODIC);
    lapic_write(LAPIC_TIMER_ICR_OFF, initial_count);

    kprintf("Calibrated and initialized apic timer");

    __asm__ __volatile__ ("sti");
}