#ifndef __ARCH_CSKY_OS_TIME_H_
#define __ARCH_CSKY_OS_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/time.h>

#ifdef __NEWLIB__
#include <sys/timespec.h>


#if !defined(__time_t_defined) && !defined(_TIME_T_DECLARED)
typedef	uint64	time_t;
#define	__time_t_defined
#define	_TIME_T_DECLARED
#endif

#endif

#ifndef _USECONDS_T_DECLARED
typedef	unsigned long useconds_t;	/* microseconds (unsigned) */
#define	_USECONDS_T_DECLARED
#endif

#if !defined(_CLOCKID_T_DECLARED)
typedef	unsigned long clockid_t;
#define	__clockid_t_defined
#define	_CLOCKID_T_DECLARED
#endif

#if !defined(__timer_t_defined) && !defined(_TIMER_T_DECLARED)
typedef	unsigned long	timer_t;
#define	__timer_t_defined
#define	_TIMER_T_DECLARED
#endif

#ifndef _SYS_TIMESPEC_H_
struct itimerspec {
    struct timespec it_interval; /**< Timer period. */
    struct timespec it_value;    /**< Timer expiration. */
};
#endif

/*struct timeval */
/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
#ifndef timerisset
#define timerisset(tvp)         ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifndef timerclear
#define timerclear(tvp)         ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

#ifndef timercmp
#define timercmp(a, b, CMP)                                                   \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))
#endif

#if 1 //see: posix/timer.c

#ifndef timeradd
#define timeradd(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
      }                                                                       \
  } while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

#endif


extern int settimeofday(const struct timeval *tv, const struct timezone *tz);

#ifdef __cplusplus
}
#endif

#endif


