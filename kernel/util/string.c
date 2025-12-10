#include "string.h"

size_t strlen(const char* str){
    size_t len = 0;
    while (str[len] != '\0'){
        len++;
    }
    return len;
}

void memcpy(void* dest, const void* src, size_t n){
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++){
        d[i] = s[i];
    }
}