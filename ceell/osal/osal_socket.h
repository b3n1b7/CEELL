/*
 * CEELL OSAL — Socket API
 *
 * Wraps BSD-style socket operations. On Zephyr, wraps the zsock_* API.
 * On POSIX systems, these map to standard socket calls.
 *
 * REQ-OSAL-005 — Socket abstraction API
 */

#ifndef CEELL_OSAL_SOCKET_H
#define CEELL_OSAL_SOCKET_H

#include <stddef.h>

/* Platform-specific includes for socket types and constants */
#if defined(__ZEPHYR__)
#include <zephyr/net/socket.h>

/* Socket types and constants are already defined by Zephyr's socket.h:
 * AF_INET, SOCK_DGRAM, SOCK_STREAM, IPPROTO_UDP, IPPROTO_TCP,
 * SOL_SOCKET, SO_REUSEADDR, IPPROTO_IP, IP_ADD_MEMBERSHIP,
 * struct sockaddr, struct sockaddr_in, struct ip_mreqn, socklen_t, etc.
 */

#endif /* __ZEPHYR__ */

/**
 * Create a socket.
 * @return socket fd on success, negative errno on failure
 */
int ceell_socket(int family, int type, int proto);

/**
 * Bind a socket to a local address.
 */
int ceell_bind(int sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Connect a socket to a remote address.
 */
int ceell_connect(int sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Close a socket.
 */
int ceell_close(int sock);

/**
 * Send data to a specific destination.
 */
int ceell_sendto(int sock, const void *buf, size_t len, int flags,
		 const struct sockaddr *dest, socklen_t addrlen);

/**
 * Receive data and source address.
 */
int ceell_recvfrom(int sock, void *buf, size_t len, int flags,
		   struct sockaddr *src, socklen_t *addrlen);

/**
 * Set a socket option.
 */
int ceell_setsockopt(int sock, int level, int optname,
		     const void *optval, socklen_t optlen);

/**
 * Convert an IP address string to binary form.
 */
int ceell_inet_pton(int family, const char *src, void *dst);

#endif /* CEELL_OSAL_SOCKET_H */
