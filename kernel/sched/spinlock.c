#include "spinlock.h"

static inline flags_t _lock_disable_int(void){
    flags_t flags = {0};

    __asm__ __volatile__("pushfq\n\tpopq %0" : "=r" (flags));
    __asm__ __volatile__("cli" : : : "memory");

    return flags;
}

static inline void _lock_enable_int(flags_t flags){
    if (flags & 0x200){
        __asm__ __volatile__ ("sti" : : : "memory");
    }
}

flags_t spinlock_aquire(spinlock_t* lock){
    flags_t flags = _lock_disable_int();
    int expected = 1;

    __asm__ __volatile__ (
        "1: "
        "lock xchgl %0, %1\n\t"
        "cmp $0, %0\n\t"
        "jne 1b"
        : "=r" (expected)
        : "m" (*lock), "0" (expected)
        : "memory"
    );

    return flags;
}

void spinlock_release(spinlock_t* lock, flags_t flags){
    __sync_synchronize();
    *lock = 0;

    _lock_enable_int(flags);
}

