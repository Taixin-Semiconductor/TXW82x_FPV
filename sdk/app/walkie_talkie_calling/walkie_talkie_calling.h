#ifndef _WALKIE_TALKIE_CALLING_H_
#define _WALKIE_TALKIE_CALLING_H_

enum {
    calling_none,
    calling_stop,
    calling_start,
};

enum {
    status_none,
    wait_connect,
    wait_accept_connect,
    accept_connect,
    connecting,
    wait_disconnect,
    disconnecting,
};

int32 walkie_talkie_calling_init(void *cb_func);
int32 walkie_talkie_calling_set(uint32 ctrl);
int32 walkie_talkie_calling_get(void);

#endif