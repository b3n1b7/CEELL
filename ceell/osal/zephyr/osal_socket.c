/*
 * CEELL OSAL — Zephyr Socket Implementation
 */

#include "../osal_socket.h"
#include <zephyr/net/socket.h>

int ceell_socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

int ceell_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

int ceell_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int ceell_close(int sock)
{
	return zsock_close(sock);
}

int ceell_sendto(int sock, const void *buf, size_t len, int flags,
		 const struct sockaddr *dest, socklen_t addrlen)
{
	return zsock_sendto(sock, buf, len, flags, dest, addrlen);
}

int ceell_recvfrom(int sock, void *buf, size_t len, int flags,
		   struct sockaddr *src, socklen_t *addrlen)
{
	return zsock_recvfrom(sock, buf, len, flags, src, addrlen);
}

int ceell_setsockopt(int sock, int level, int optname,
		     const void *optval, socklen_t optlen)
{
	return zsock_setsockopt(sock, level, optname, optval, optlen);
}

int ceell_inet_pton(int family, const char *src, void *dst)
{
	return zsock_inet_pton(family, src, dst);
}
