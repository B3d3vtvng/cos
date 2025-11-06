#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

#define SPINLOCK_INIT 0

typedef int spinlock_t;
typedef uint64_t flags_t;

flags_t spinlock_aquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock, flags_t flags);

#endif