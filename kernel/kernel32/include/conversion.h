#ifndef __CONVERSION_H__
#define __CONVERSION_H__

void int_to_str32(int value, char* str, int base);
int str_to_int32(const char* str, int base);
void uint_to_str32(unsigned int value, char* str, int base);
unsigned int str_to_uint32(const char* str, int base);

#endif