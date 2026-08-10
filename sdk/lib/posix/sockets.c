#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/sleep.h"
#include "lwip/netdb.h"
#include "lwip/if_api.h"

#ifdef TXWSDK_POSIX

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return lwip_accept(s, addr, addrlen);
}

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_bind(s, name, namelen);
}

int shutdown(int s, int how)
{
    return lwip_shutdown(s, how);
}

int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getpeername(s, name, namelen);
}

int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getsockname(s, name, namelen);
}

int setsockopt(int s, int level, int optname, const void *opval, socklen_t optlen)
{
    return lwip_setsockopt(s, level, optname, opval, optlen);
}

int getsockopt(int s, int level, int optname, void *opval, socklen_t *optlen)
{
    return lwip_getsockopt(s, level, optname, opval, optlen);
}

int closesocket(int s)
{
    return lwip_close(s);
}

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_connect(s, name, namelen);
}

int listen(int s, int backlog)
{
    return lwip_listen(s, backlog);
}

ssize_t recv(int s, void *mem, size_t len, int flags)
{
    return lwip_recv(s, mem, len, flags);
}

ssize_t recvmsg(int s, struct msghdr *message, int flags)
{
    return lwip_recvmsg(s, message, flags);
}

ssize_t recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    return lwip_recvfrom(s, mem, len, flags, from, fromlen);
}

ssize_t send(int s, const void *dataptr, size_t size, int flags)
{
    return lwip_send(s, dataptr, size, flags);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
    return lwip_sendmsg(s, msg, flags);
}

ssize_t sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return lwip_sendto(s, data, size, flags, to, tolen);
}

int socket(int domain, int type, int protocol)
{
    return lwip_socket(domain, type, protocol);
}

int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout)
{
    return lwip_select(maxfdp1, readset, writeset, exceptset, timeout);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    return lwip_poll(fds, nfds, timeout);
}

int ioctlsocket(int s, long cmd, void *argp)
{
    return lwip_ioctl(s, cmd, argp);
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    return lwip_inet_ntop(af, src, dst, size);
}

int inet_pton(int af, const char *src, void *dst)
{
    return lwip_inet_pton(af, src, dst);
}

char *if_indextoname(unsigned int ifindex, char *ifname)
{
    return lwip_if_indextoname(ifindex, ifname);
}

unsigned int if_nametoindex(const char *ifname)
{
    return lwip_if_nametoindex(ifname);
}

struct hostent *gethostbyname(const char *name)
{
    return lwip_gethostbyname(name, 1);
}

int gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                    size_t buflen, struct hostent **result, int *h_errnop)
{
    return lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop, 1);
}

struct hostent *gethostbyname_async(const char *name)
{
    return lwip_gethostbyname(name, 0);
}

int gethostbyname_r_async(const char *name, struct hostent *ret, char *buf,
                          size_t buflen, struct hostent **result, int *h_errnop)
{
    return lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop, 0);
}

void freeaddrinfo(struct addrinfo *ai)
{
    lwip_freeaddrinfo(ai);
}

int getaddrinfo(const char *nodename, const char *servname,
                const struct addrinfo *hints, struct addrinfo **res)
{

    return lwip_getaddrinfo(nodename, servname, hints, res);
}
                
#endif

