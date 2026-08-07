/**@file    eloop.c
 * @author  何凱帆
 * @brief   基於socket的event loop服務
 * @details 提供基於socket的event loop服務
 *
 * 系統依賴：
 * - time
 * - socket
 * 這些依賴在plat-cfg.h的system time config和socket config中解決，單獨移植時可從中抽取
 *
 * 統一使用list.h來管理雙向鏈表
 */
#include "sys_config.h"
#include "typesdef.h" 
#include "list.h" 
#include "lib/net/eloop/eloop.h"
#include "osal/irq.h"
#include <k_api.h>
//#include "osal/osal_time.h"

#define eloop_dbg(fmt, ...) _os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define eloop_err(fmt, ...) _os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ~0
#endif

#if ELOOP_WAKEUP_BY_LOOPBACK

static int global_loopback_socket;
#endif
static uint8_t global_loopback_flag = 0;	//读标志,防止自己唤醒过多,保证尽量多个线程也是只是唤醒一次


//#include "debug.h"
static void eloop_wakeup(void);



static struct eloop_state els;



/*******************************************************************************
 * time
 ******************************************************************************/
int time_diff(eloop_time_ref* tr_start, eloop_time_ref* tr_end)
{
	return (*tr_end) - (*tr_start);

}

int time_ago(eloop_time_ref* tr)
{
	eloop_time_ref now;
	now = get_time_now();
	return time_diff(&now,tr);


}

void time_now(eloop_time_ref* tr)
{
  *tr = get_time_now();
}

//单位是ms
unsigned int get_time_now()
{
  unsigned int now_time;
  now_time = os_jiffies();
  return now_time;
}


void time_add(eloop_time_ref* tr, int msec)
{
	*tr += (msec);
}

void time_future(eloop_time_ref* tr, int msec)
{
	*tr = get_time_now();
	time_add(tr,msec);
}

/*******************************************************************************
 * eloop
 ******************************************************************************/
static struct event* eloop_new_event(event_callback f, void* d)
{
  struct event* e;

  e = (struct event*)malloc(sizeof(struct event));
  if(!e)
  {
    eloop_err("malloc fail\n");
    return e;
  }
  INIT_LIST_HEAD(&e->list);
  e->type = 0;
  e->flags = 0;
  e->func = f;
  e->data = d;
  return e;
}

static void eloop_strip_events(struct list_head* list)
{
  struct list_head* pos, *n;
  struct event* e;

  list_for_each_safe(pos, n, list)
  {
    e = list_entry(pos, struct event, list);
    if(e->flags & EVENT_F_REMOVE) {
      list_del_init(pos);
      free(e); // todo: 要確定誰來負責釋放空間
    }
  }
}

//轮询wait事件(没有添加到对应列表的事件),这里只是管添加,不管事件的状态
static int eloop_wait_events(struct list_head* list)
{
  struct list_head* pos, *n;
  struct event* e;
  int count = 0;
  uint32_t flag;
  
  list_for_each_safe(pos, n, list)
  {
    e = list_entry(pos, struct event, list);
    flag = disable_irq();
    list_del_init(pos);
    enable_irq(flag);
    count++;
    switch(e->type)
    {
      case EVENT_TIME:
        list_add(&e->list, &els.times);
      break;
      case EVENT_FD:
        list_add(&e->list, &els.fds);
      break;
      case EVENT_ALWAYS:
        list_add(&e->list, &els.always);
      break;
      default:
        eloop_err("%s:%d err\n",__FUNCTION__,__LINE__);
      break;
    }
  }
  return count;
}


void eloop_resched_event(EVT_HDL ev, eloop_time_ref* tr)
{
  struct event* e = (struct event*)ev;
  if(tr)
    e->ev.time.fire = *tr;
  else if(e->flags & EVENT_F_ENABLED)
    time_add(&e->ev.time.fire, e->ev.time.ival);
  else
    time_future(&e->ev.time.fire, e->ev.time.ival);

  e->flags &= ~EVENT_F_REMOVE;
  //e->flags |= EVENT_F_ENABLED;
}

void set_event_time(void *d,int msec)
{
	struct event* e = (struct event*)d;
	e->ev.time.ival = msec;
}

EVT_HDL eloop_add_timer(int msec, unsigned int flags, event_callback f, void* d)
{
  struct event* e;

  e = eloop_new_event(f, d);
  if(!e)
  {
    return e;
  }
  e->type = EVENT_TIME;
  e->flags = flags;
  e->ev.time.ival = msec;
  
  time_now(&e->ev.time.fire);
  eloop_resched_event(e, NULL);
  uint32_t flag = disable_irq();
  list_add(&e->list, &els.wait);
  enable_irq(flag);
  eloop_wakeup();
  return e;
}
/****************************************************************************************
arg:
	t:执行的时间,类似闹钟

注意:调用该函数,默认就是EVENT_F_ONESHOT,但是是否启动通过flags来配置
******************************************************************************************/
EVT_HDL eloop_add_alarm(eloop_time_ref t, unsigned int flags, event_callback f, void* d)
{
  struct event* e;
  eloop_time_ref timer = t;
  e = eloop_new_event(f, d);
  if(!e)
  {
    return e;
  } 
  e->type = EVENT_TIME;
  e->flags = flags | EVENT_F_ONESHOT;
  e->ev.time.ival = 0;
  
  eloop_resched_event(e, &timer);
  uint32_t flag = disable_irq();
  list_add(&e->list, &els.wait);
  enable_irq(flag);
  eloop_wakeup();
  return e;
}

EVT_HDL eloop_add_fd(SOCK_HDL fd, int write, unsigned int flags, event_callback f, void* d)
{
  struct event* e;

  e = eloop_new_event(f, d);
  if(!e)
  {
    return e;
  }
  e->type = EVENT_FD;
  e->flags = flags ;
  e->ev.fd.fd = fd;
  e->ev.fd.write = write;
  uint32_t flag = disable_irq();
  list_add(&e->list, &els.wait);
  enable_irq(flag);
  eloop_wakeup();
  
  return e;
}

EVT_HDL eloop_add_fd_init(SOCK_HDL fd, int write, unsigned int flags, event_callback f, void* d)
{
  struct event* e;

  e = eloop_new_event(f, d);
  if(!e)
  {
    return e;
  }
  e->type = EVENT_FD;
  e->flags = flags;
  e->ev.fd.fd = fd;
  e->ev.fd.write = write;
  uint32_t flag = disable_irq();
  list_add(&e->list, &els.wait);
  enable_irq(flag);
  return e;
}


EVT_HDL eloop_add_always(unsigned int flags, event_callback f, void* d)
{
  struct event* e;

  e = eloop_new_event(f, d);
  if(!e)
  {
    return e;
  }
  e->type = EVENT_ALWAYS;
  e->flags = flags | EVENT_F_ENABLED;
  uint32_t flag = disable_irq();
  list_add(&e->list, &els.wait);
  enable_irq(flag);
  eloop_wakeup();
  
  return e;
}
 
void eloop_remove_event(EVT_HDL ev)
{
  struct event* e = (struct event*)ev;
  e->flags |= EVENT_F_REMOVE;
  e->flags &= ~(EVENT_F_RUNNING | EVENT_F_ENABLED);
  eloop_wakeup();
}

void eloop_set_event_interval(EVT_HDL ev, int msec)
{
  struct event* e = (struct event*)ev;
  e->ev.time.ival = msec;
  if(e->flags & EVENT_F_ENABLED)
    eloop_resched_event(e, NULL);
}

void eloop_set_event_enabled(EVT_HDL ev, int enabled)
{
  struct event* e = (struct event*)ev;

  if(enabled)
    e->flags |= EVENT_F_ENABLED;
  else
    e->flags &= ~EVENT_F_ENABLED;

  eloop_wakeup();
}

int eloop_get_event_enabled(EVT_HDL ev)
{
  struct event* e = (struct event*)ev;
  return e->flags & EVENT_F_ENABLED ? 1 : 0;
}

void eloop_exit(void)
{
  els.end_loop = 1;
}

int eloop_wakeup_init(void);
void eloop_init(void)
{
  eloop_dbg("start\r\n");
  INIT_LIST_HEAD(&els.times);
  INIT_LIST_HEAD(&els.fds);
  INIT_LIST_HEAD(&els.always);
  INIT_LIST_HEAD(&els.wait);
  els.end_loop = 0;
  /*  */
//  if(osal_socket_startup()) {
//    p_err("Socket startups error!");
//    return;
//  }
  eloop_wakeup_init();
}


void user_eloop_run()
{
  time_ref t, *st;
  struct event* e;
  int diff, nexttime = 0, highfd, ret;
  fd_set rfds, wfds;
  int count = 0;

  els.end_loop = 0;
  eloop_dbg("run\r\n");

  do {
    struct list_head* pos, *n;
	  global_loopback_flag = 0;
    st = NULL;
    //如果有需要添加到其他list的,就将nexttime设置为0,需要立刻轮询一下
    count = eloop_wait_events(&els.wait);
    if(count)
    {
        nexttime = 0;
    }
    /* check how long the timeout should be */
    list_for_each_safe(pos, n, &els.times)
    {
      e = list_entry(pos, struct event, list);
      if(e->flags & EVENT_F_ENABLED) {
        diff = time_ago(&e->ev.time.fire);
        if(diff < 1)
          diff = 0;
        if(!st || diff < nexttime)
          nexttime = diff;
        st = &t;
        e->flags |= EVENT_F_RUNNING;
      } else
        e->flags &= ~EVENT_F_RUNNING;
    }
    list_for_each_safe(pos, n, &els.always)
    {
      e = list_entry(pos, struct event, list);
      if(e->flags & EVENT_F_ENABLED) {
        st = &t;
        nexttime = 0;
        e->flags |= EVENT_F_RUNNING;
      } else
        e->flags &= ~EVENT_F_RUNNING;
    }

    if(st) {
      st->tv_sec = nexttime / 1000;
      st->tv_usec = (nexttime % 1000) * 1000;
    }
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    highfd = -1;
    /* This is all so ugly...  It should use poll() eventually. */
    list_for_each_safe(pos, n, &els.fds)
    {
      e = list_entry(pos, struct event, list);
      if(e->flags & EVENT_F_ENABLED) {
        FD_SET(e->ev.fd.fd, e->ev.fd.write ? &wfds : &rfds);
        if(e->ev.fd.fd > highfd)
          highfd = e->ev.fd.fd;
        e->flags |= EVENT_F_RUNNING;
      } else
        e->flags &= ~EVENT_F_RUNNING;
    }
	

	
	
    ret = select(highfd + 1, &rfds, &wfds, NULL, st);
	global_loopback_flag = 1;


    list_for_each_safe(pos, n, &els.times)
    {
      e = list_entry(pos, struct event, list);
      if(!(e->flags & EVENT_F_RUNNING))
        continue;
      if(els.end_loop)
        break;
      diff = time_ago(&e->ev.time.fire);
      if(diff < 1) {
        if(!(e->flags & EVENT_F_ONESHOT))
          eloop_resched_event(e, NULL);
        else
          e->flags |= EVENT_F_REMOVE;
        (*e->func)(e, e->data);
      }
    }
    list_for_each_safe(pos, n, &els.always)
    {
      e = list_entry(pos, struct event, list);
      if(!(e->flags & EVENT_F_RUNNING))
        continue;
      if(els.end_loop)
        break;
      if(e->flags & EVENT_F_ONESHOT)
        e->flags |= EVENT_F_REMOVE;
      (*e->func)(e, e->data);
    }
    if(ret > 0)		
      list_for_each_safe(pos, n, &els.fds)
      {
        e = list_entry(pos, struct event, list);
        if(!(e->flags & EVENT_F_RUNNING))
          continue;
        if(els.end_loop)
          break;
        if(FD_ISSET(e->ev.fd.fd, e->ev.fd.write ? &wfds : &rfds)) {
          if(e->flags & EVENT_F_ONESHOT)
            e->flags |= EVENT_F_REMOVE;
          (*e->func)(e, e->data);
        }
      }
    eloop_strip_events(&els.times);
    eloop_strip_events(&els.fds);
    eloop_strip_events(&els.always);
  } while(!els.end_loop );
}

#if ELOOP_WAKEUP_BY_LOOPBACK




struct udp_loopback
{
	int version;//版本号,版本一致才能正常接收
	int port;//端口号
	unsigned int size;
	int crc;//保留crc校验机制
};


static void eloop_wakeup_read(struct event *ei, void *d)
{
	int socket = (int)d;
	struct udp_loopback loopback;
	struct sockaddr_in remote_addr;
	socklen_t retval = (sizeof(struct sockaddr_in));
	recvfrom(socket, (char*)&loopback, sizeof(struct udp_loopback), 0,(struct sockaddr *) &remote_addr, &retval);
	global_loopback_flag = 0;
}


int eloop_wakeup_init(void)
{
	int err;
	int t_socket;
	struct sockaddr_in listenAddr;
  struct sockaddr_in connect_addr;
  socklen_t namelen = sizeof(connect_addr);
	memset(&listenAddr, 0, sizeof(struct sockaddr_in));
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	
	listenAddr.sin_port = htons(0);
	t_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (t_socket == INVALID_SOCKET)
	{
		eloop_err("socket error !");
		return 1;
	}

	err = bind(t_socket, (struct sockaddr*) &listenAddr, sizeof(struct sockaddr_in));
  global_loopback_socket = t_socket;
  getsockname( t_socket, (struct sockaddr *)&connect_addr, &namelen );
  connect( t_socket, (struct sockaddr *)&connect_addr,sizeof( connect_addr ) );
  eloop_add_fd_init( t_socket, EVENT_READ, EVENT_F_ENABLED, (void*)eloop_wakeup_read, (void*)t_socket );

  return 0;
}


static void eloop_wakeup(void)
{

  struct udp_loopback loopback;
  if(global_loopback_flag)
  {
	  return;
  }
  global_loopback_flag = 1;
  //sendto(global_loopback_socket, (char*)&loopback, sizeof(loopback), 0, (struct sockaddr*)&global_loop_addr, sizeof (global_loop_addr));
  send(global_loopback_socket, (char*)&loopback, sizeof(loopback), 0);
}

#else  /* ELOOP_WAKEUP_BY_LOOPBACK */
// loopback機制不可用的話，採用定時事件來喚醒
static void eloop_wakeup_read(EVT_HDL e, void* d)
{
  _os_printf ("eloop wakeup\r\n");
}

int eloop_wakeup_init(void)
{
  EVT_HDL e;
  e = eloop_add_timer (5000, EVENT_F_ENABLED, eloop_wakeup_read, NULL);
}

static void eloop_wakeup(void) {}
#endif  /* ELOOP_WAKEUP_BY_LOOPBACK */
