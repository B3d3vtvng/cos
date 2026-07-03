#ifndef PROC_H
#define PROC_H

#include <stdint.h>

typedef struct proc_state{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags;
    uint64_t rsp, ss;
} __attribute__((packed));

#endif