#include "MQTT_port.h"
#include "osal/string.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"



void MutexInit(Mutex* mutex)
{
	os_mutex_init(mutex);
}

int MutexLock(Mutex* mutex)
{
	os_mutex_lock(mutex, osWaitForever);
	return 0;
}

int MutexUnlock(Mutex* mutex)
{
	os_mutex_unlock(mutex);
	return 0;
}

void TimerInit(Timer* timer)
{
	timer->ticks = 0;
}

char TimerIsExpired(Timer* timer)
{
	char ret = 0;
	if (timer->ticks <= os_jiffies())
	{
		ret = 1;
	}
	return ret;
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
	timer->ticks = timeout_ms + os_jiffies();
}


void TimerCountdown(Timer* timer, unsigned int timeout)
{
	timer->ticks = (timeout*1000) + os_jiffies();
}


int TimerLeftMS(Timer* timer)
{
	if (timer->ticks > os_jiffies())
	{
		return (timer->ticks - os_jiffies());
	}
	return 0;
}



static int mqtt_lwip_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	#if 0
	struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
	if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
	{
		interval.tv_sec = 0;
		interval.tv_usec = 100;
	}
	os_printf("timeout_ms:%d\n",timeout_ms);
	#endif
	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_ms, sizeof(timeout_ms));

	int bytes = 0;
	while (bytes < len)
	{
		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
		if (rc == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			  bytes = -1;
			break;
		}
		else if (rc == 0)
		{
			bytes = 0;
			break;
		}
		else
			bytes += rc;
	}
	return bytes;
}




static int mqtt_lwip_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	#if 0
	struct timeval tv;

	tv.tv_sec = 0;  /* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors
	#endif
	setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout_ms,sizeof(timeout_ms));
	int	rc = write(n->my_socket, buffer, len);
	os_printf("%s:%d\trc:%d\n",__FUNCTION__,__LINE__,rc);
	return rc;
}

void NetworkInit(Network* n)
{
	n->my_socket = 0;
	n->mqttread = mqtt_lwip_read;
	n->mqttwrite = mqtt_lwip_write;
}

int NetworkConnect(Network* n, char* addr, int port)
{
	int type = SOCK_STREAM;
	struct sockaddr_in address;
	int rc = -1;
	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
	{
		struct addrinfo* res = result;

		/* prefer ip4 addresses */
		while (res)
		{
			if (res->ai_family == AF_INET)
			{
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET)
		{
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
			address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		}
		else
			rc = -1;

		freeaddrinfo(result);
	}

	if (rc == 0)
	{
		n->my_socket = socket(family, type, 0);

		if (n->my_socket != -1)
		{
			int keepalive = 1;
			setsockopt(n->my_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
			fcntl( n->my_socket, F_SETFL, O_NONBLOCK );
			rc = connect_tmo(n->my_socket, (struct sockaddr*)&address, sizeof(address),1000);
			fcntl( n->my_socket, F_SETFL, ~O_NONBLOCK );
		}
			
		else
			rc = -1;
	}

	return rc;
}

void NetworkDisconnect(Network* n)
{
	close(n->my_socket);
}
