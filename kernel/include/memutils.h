#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

void memset(void* dest, char val, unsigned long len);
void memcpy(void* dest, const void* src, unsigned long len);
int memcmp(const void* s1, const void* s2, unsigned long len);

#endif