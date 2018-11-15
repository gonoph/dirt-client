#include "String.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int String::printf(const char *fmt, ...) {
    char buf[4096];
    int res;
    
    va_list va;
    va_start(va, fmt);

    res = vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);
    replace(buf);

    return res;
}
