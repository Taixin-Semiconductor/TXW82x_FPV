#ifndef __MQTT_PORT_H
#define __MQTT_PORT_H
#include "typesdef.h"
#include "osal/mutex.h"
#include "osal/task.h"

#ifndef NULL
#define NULL (void *)0
#endif
typedef struct Timer 
{
    uint32_t ticks;
} Timer;

typedef struct Network Network;
struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};


typedef struct os_mutex Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);


void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

void NetworkInit(Network*);
int NetworkConnect(Network*, char*, int);
void NetworkDisconnect(Network*);

#endif