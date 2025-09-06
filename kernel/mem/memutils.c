#include "memutils.h"

void memset(void* dest, char val, unsigned long len) {
    unsigned char* ptr = (unsigned char*)dest;
    for (unsigned long i = 0; i < len; i++) {
        ptr[i] = (unsigned char)val;
    }
}

void memcpy(void* dest, const void* src, unsigned long len) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (unsigned long i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

int memcmp(const void* s1, const void* s2, unsigned long len) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    for (unsigned long i = 0; i < len; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }
    return 0;
}