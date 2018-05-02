#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <stdio.h>
#include <stdarg.h>

void
debug_printf(const char *format, ...)
{
#ifndef NDEBUG
   va_list args;

   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
#endif
}

#endif // DEBUG_HPP
