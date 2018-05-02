#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#define TIMER_BEGIN \
  do { \
    struct timeval timer_begin; \
    gettimeofday(&timer_begin, NULL); \
    do

#define TIMER_END(...) \
    while (0); \
    struct timeval timer_end; \
    gettimeofday(&timer_end, NULL); \
    double timer = (timer_end.tv_sec - timer_begin.tv_sec) + \
      ((double)(timer_end.tv_usec - timer_begin.tv_usec) / (1000 * 1000)); \
    debug_printf(__VA_ARGS__); \
  } while (0)


bool print_debug = true;

// suppress debug output for a single statement
#define PAUSE_DEBUG \
  for (bool _print_debug_old = print_debug, _end_me = true; \
    (print_debug = (_print_debug_old && !_end_me)), _end_me; _end_me = false)

void
debug_printf(const char *format, ...)
{
#ifndef NDEBUG
   va_list args;

   va_start(args, format);
   if (print_debug) vfprintf(stderr, format, args);
   va_end(args);
#endif
}

#endif // DEBUG_HPP
