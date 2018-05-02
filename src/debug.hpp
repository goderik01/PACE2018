#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <assert.h>
#include <random>

#define rand() better_rand()

std::mt19937_64 _rand_gen(0);
int better_rand() {
  static std::uniform_int_distribution<int> dist{0, std::numeric_limits<int>::max()};
  return dist(_rand_gen);
}


#define CHECK_SIGNALS(cmd) ({ if (g_stop_signal != 0 || MyTimer::triggered()) { cmd; } })

struct MyTimer {
  static long long end;

  MyTimer(long long timeout) {
    assert(end == 0);
    struct timeval now;
    gettimeofday(&now, NULL);
    end = now.tv_sec + timeout;
  }

  ~MyTimer() { end = 0; }

  static bool triggered() {
    if (end == 0) return false;
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec >= end;
  }

  operator bool() const { return true; }
};
long long MyTimer::end;

#define TIMER(timeout) if (auto _timer = MyTimer(timeout))


#define Assert(cond, ...) do { \
    if (!(cond)) { \
      fprintf(stderr, __VA_ARGS__); \
      assert(0); \
    } \
  } while (0)

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

#ifndef NDEBUG
void
debug_printf(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   if (print_debug) vfprintf(stderr, format, args);
   va_end(args);
}
#else
#define debug_printf(...) do {} while(0)
#endif

#endif // DEBUG_HPP
