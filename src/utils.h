#ifndef _UTILS_H
#define _UTILS_H

#include <Arduino.h>

inline bool is_double_digit_hour_in_12h(uint8_t hour) {
    if (hour == 0)
        return true;

    if (10 <= hour && hour <= 12)
        return true;

    if (22 <= hour && hour <= 24)
        return true;

    return false;
}

inline float celsius_to_fahrenheit(float c) {
    return (c * 1.8) + 32.0;
}

inline void str_to_uppper(char* pch) {
    while (*pch != '\0') {
        *pch = toupper(*pch);
        pch++;
    }
}

#endif
