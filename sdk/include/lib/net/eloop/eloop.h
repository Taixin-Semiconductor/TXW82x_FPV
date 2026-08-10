/**@file    eloop.h
 * @author  何凱帆
 * @brief   基於socket的event loop服務
 * @details 提供基於socket的event loop服務
 */

#ifndef _ELOOP_H_
#define _ELOOP_H_




#include "list.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

#define EVT_HDL void *

#ifndef SOCK_HDL
#define SOCK_HDL int
#endif


#define EVENT_READ  0
#define EVENT_WRITE 1

#define EVENT_F_DISABLED 0
#define EVENT_F_NOACTION    0
#define EVENT_F_ENABLED 1
#define EVENT_F_REMOVE 2
#define EVENT_F_ONESHOT 4
#define EVENT_F_RUNNING 8



#define ELOOP_WAKEUP_BY_LOOPBACK 1

#define EVENT_TIME 1
#define EVENT_FD 2
#define EVENT_ALWAYS 3
#define EVENT_SOFT_QUEUE 4


struct eloop_timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};




typedef struct timeval time_ref;
typedef unsigned int eloop_time_ref;


typedef void (*event_callback)(EVT_HDL e, void* d);





struct time_event {
  eloop_time_ref fire;
  int ival;
};

struct fd_event {
  SOCK_HDL fd;
  int write; // 0 = read, 1 = write
};



struct eloop_state {
  struct list_head times;
  struct list_head fds;
  struct list_head always;
  struct list_head wait;
  int end_loop;
};


struct event {
  struct list_head list;
  event_callback func;
  void* data;
  int type;
  int flags;
  union {
    struct time_event time;
    struct fd_event fd;
  } ev;
};

//#include "linux_ktime.h"





void eloop_resched_event(EVT_HDL ev, eloop_time_ref* tr);
void eloop_remove_event(EVT_HDL ev);
void eloop_set_event_interval(EVT_HDL ev, int msec);
void eloop_set_event_enabled(EVT_HDL ev, int enabled);
int eloop_get_event_enabled(EVT_HDL ev);

EVT_HDL eloop_add_timer(int msec, unsigned int flags, event_callback f, void* d);
EVT_HDL eloop_add_alarm(eloop_time_ref t, unsigned int flags, event_callback f, void* d);
EVT_HDL eloop_add_fd(SOCK_HDL fd, int write, unsigned int flags, event_callback f, void* d);
EVT_HDL eloop_add_always(unsigned int flags, event_callback f, void* d);
void set_event_time(void *d,int msec);


void eloop_exit(void);
void eloop_init(void);
void user_eloop_run();
static void eloop_wakeup(void);

void time_now(eloop_time_ref* tr);
unsigned int get_time_now();
//int gettimeofday(struct timeval *tv,void *ignore);



#endif /* _ELOOP_H_ */
