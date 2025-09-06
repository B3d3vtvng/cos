#ifndef __CONVERSION_H__
#define __CONVERSION_H__

void int_to_str(int value, char* str, int base);
int str_to_int(const char* str, int base);
void uint_to_str(unsigned int value, char* str, int base);
unsigned int str_to_uint(const char* str, int base);

#endif