//
// Created by Michal Kowalik on 18.02.24.
//

#include <stdio.h>
#include <stdarg.h>

// set to false to disable debug output
#define  DEBUG_OUTPUT true

void debug_printf(const char *format, ...) {
#ifdef DEBUG_OUTPUT
    va_list args;
    char buffer[256];
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);
    printf("%s", buffer);
#endif
}
