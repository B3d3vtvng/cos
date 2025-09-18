#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

void memset32(void* dest, char val, unsigned long len);
void memcpy32(void* dest, const void* src, unsigned long len);
int memcmp32(const void* s1, const void* s2, unsigned long len);

#endif