#include "conversion.h"

int ltoa(long num, char* out){
    if (out == NULL){
        // If null is given, return the length of the number string
        int length = 0;
        int n = num;
        if (n <= 0) {
            length++; // For '-' sign or '0'
        }

        while (n != 0) {
            length++;
            n /= 10;
        }
        return length;
    }

    if (num == 0) {
        out[0] = '0';
        out[1] = '\0';
        return 0;
    }

    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    char buffer[20];
    int i = 0;
    while (num != 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    // Reverse the string
    for (int j = 0; j < i; j++) {
        out[j] = buffer[i - j - 1];
    }
    out[i] = '\0';

    return 0;
}

int ltohex(long num, char* out){
    if (out == NULL){
        return 18; // Fixed length for hex representation of long
    }

    out[0] = '0';
    out[1] = 'x';

    const char* hex_chars = "0123456789ABCDEF";
    for (int i = 17; i >= 2; i--) {
        out[i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    out[18] = '\0';

    return 0;
}

