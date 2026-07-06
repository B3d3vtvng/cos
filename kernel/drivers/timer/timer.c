#include "timer.h"

static spinlock_t timer_lock = SPINLOCK_INIT;
static struct notify_queue* nqueue = NULL;
static uint32_t id_next = 0;
static uint32_t apic_timer_period = 0;
static bool use_pit_timer = false;
static bool sleep = false;

#define PIT_BASE_FREQ 1193182U

extern void isr_timer(void);

#define CALIB_SLEEP_PIT_TICKS 11932
#define LAPIC_TIMER_PERIODIC (1 << 17)
#define TARGET_HZ 100
#define LAPIC_TIMER_FALLBACK_TICKS_10MS 625000U
#define LAPIC_TIMER_FALLBACK_PERIOD 625000U


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
            void (*callback)(struct proc_state*) = cur->notify_callback;
            bool once = cur->once;

            if (once) {
                struct notify_queue* to_remove = cur;
                if (prev == NULL) {
                    nqueue = cur->next;
                    cur = nqueue;
                } else {
                    prev->next = cur->next;
                    cur = cur->next;
                }
                spinlock_release(&timer_lock, flags);
                callback(pstate);
                kfree(to_remove);
                flags = spinlock_aquire(&timer_lock);
                continue;
            }

            cur->ticks_cur = cur->ticks;
            spinlock_release(&timer_lock, flags);
            callback(pstate);
            flags = spinlock_aquire(&timer_lock);
        }

        prev = cur;
        if (cur) cur = cur->next;
    }

    lapic_write(LAPIC_EOI_REG_OFF, 0);
    if (use_pit_timer) {
        pic_eoi_master();
    }
    spinlock_release(&timer_lock, flags);
}

static uint16_t pit_read_count(void){
    io_outb(0x43, 0);
    uint8_t lo = io_inb(0x40);
    uint8_t hi = io_inb(0x40);
    return lo | (hi << 8);
}

void pit_prepare_sleep(void){
    io_outb(0x43, 0x30);
    io_outb(0x40, (uint8_t) (CALIB_SLEEP_PIT_TICKS & 0xFF));
    io_outb(0x40, (uint8_t) ((CALIB_SLEEP_PIT_TICKS >> 8) & 0xFF));
}

static void pit_configure_hz(uint32_t hz){
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / hz);
    io_outb(0x43, 0x36);
    io_outb(0x40, (uint8_t)(divisor & 0xFF));
    io_outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

static uint64_t idt_gate_handler(struct idt_entry* gate){
    return gate->offset_low
        | ((uint64_t)gate->offset_mid << 16)
        | ((uint64_t)gate->offset_high << 32);
}

void timer_init(struct idt_ptr* idtp){
    idt_set_gate((struct idt_entry*) idtp->base, 0x20, (uint64_t) isr_timer, 0x08, 0x8E);

    struct idt_entry* gate = &((struct idt_entry*) idtp->base)[0x20];
    if (idt_gate_handler(gate) != (uint64_t)isr_timer || gate->selector != 0x08) {
        kernel_panic(NULL, "Timer IDT gate setup failed\n");
    }

    lapic_timer_write(LAPIC_TIMER_DCR_OFF, 0x3);
    io_wait();

    pit_prepare_sleep();
    while (pit_read_count() == 0xFFFF);

    lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_INT_MASK | TIMER_LVT_ONE_SHOT | 0x20);
    io_wait();
    lapic_timer_write(LAPIC_TIMER_ICR_OFF, 0xFFFFFFFF);
    io_wait();

    uint32_t ticks_in_10ms = 0;
    uint32_t ccr_start = lapic_timer_read(LAPIC_TIMER_CCR_OFF);
    if (ccr_start != 0) {
        lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_ONE_SHOT | 0x20);
        io_wait();

        while (pit_read_count() != 0);

        uint32_t remaining = lapic_timer_read(LAPIC_TIMER_CCR_OFF);
        if (remaining == 0 || remaining >= ccr_start) {
            kernel_panic(NULL, "APIC timer calibration failed\n");
        }

        ticks_in_10ms = ccr_start - remaining;
        lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_INT_MASK | TIMER_LVT_ONE_SHOT | 0x20);
        io_wait();
    } else {
        use_pit_timer = true;
        ticks_in_10ms = LAPIC_TIMER_FALLBACK_TICKS_10MS;
        kprintf("APIC timer CCR unreadable, using PIT fallback %u ticks/10ms\n", ticks_in_10ms);
    }

    uint64_t initial_count = ((uint64_t)ticks_in_10ms * 100ULL) / TARGET_HZ;

    if (ticks_in_10ms == 0 || ticks_in_10ms > 100000000
            || initial_count == 0 || initial_count >= 0xFFFFFFFFULL) {
        apic_timer_period = LAPIC_TIMER_FALLBACK_PERIOD;
    } else {
        apic_timer_period = (uint32_t)initial_count;
    }

    lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_INT_MASK | TIMER_LVT_PERIODIC | 0x20);
    lapic_timer_write(LAPIC_TIMER_ICR_OFF, apic_timer_period);

    kprintf("APIC timer calibrated: %u ticks/10ms, period=%u\n",
            ticks_in_10ms, apic_timer_period);

    lidt(idtp);
}

void timer_start(void){
    if (use_pit_timer) {
        lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_INT_MASK | TIMER_LVT_PERIODIC | 0x20);
        pic_remap(0x20, 0x28);
        lapic_enable_pic_passthrough();
        pit_configure_hz(TARGET_HZ);
        pic_unmask_irq0();
    } else {
        lapic_timer_write(LAPIC_LVT_TIMER_REG_OFF, TIMER_LVT_PERIODIC | 0x20);
        io_wait();
        lapic_timer_write(LAPIC_TIMER_ICR_OFF, apic_timer_period);
        io_wait();
    }

    __asm__ __volatile__("sti" ::: "memory");
}

void timer_busy_wait_end(struct proc_state* pstate){
    sleep = false;
}

void timer_busy_wait(uint64_t ms){
    sleep = true;
    notify_add(ms, timer_busy_wait_end);
    while (sleep){}
    return;
}