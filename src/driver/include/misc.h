/* MIT License

Copyright (c) 2022 Po Jui Shih
Copyright (c) 2022 Hassaan Saadat
Copyright (c) 2022 Sri Parameswaran
Copyright (c) 2022 Hasindu Gamaarachchi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <stdint.h>

#define INFO_PREFIX "[%s::INFO]\033[1;34m "
#define ERROR_PREFIX "[%s::ERROR]\033[1;31m "
#define WARNING_PREFIX "[%s::WARNING]\033[1;33m "
#define NO_COLOUR "\033[0m"

// TODO: add defines of error codes

/*
 * Register getter and setter
 */
#define _reg_set(BaseAddress, RegOffset, Data) \
    *(volatile uint32_t*)((BaseAddress) + (RegOffset >> 2)) = (uint32_t)(Data)
#define _reg_get(BaseAddress, RegOffset) \
    *(volatile uint32_t*)((BaseAddress) + (RegOffset >> 2))

#define INFO(msg, ...) \
    fprintf(stderr, INFO_PREFIX msg NO_COLOUR "\n", __func__, __VA_ARGS__); 

#define ERROR(msg, ...) \
    fprintf(stderr, ERROR_PREFIX msg NO_COLOUR \
        " At %s:%d\n", \
        __func__, __VA_ARGS__, __FILE__, __LINE__ - 1); \

#define WARNING(msg, ...) \
    fprintf(stderr, WARNING_PREFIX msg NO_COLOUR \
            " At %s:%d\n", \
            __func__, __VA_ARGS__, __FILE__, __LINE__ - 1); \

#define MALLOC_CHK(ret) { \
    if ((ret) == NULL) { \
        MALLOC_ERROR() \
    } \
}

#define MALLOC_ERROR() ERROR("%s", "Failed to allocate memory.")

#endif // MISC_H