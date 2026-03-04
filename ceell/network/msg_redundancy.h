/*
 * CEELL Dual-Path Redundant Messaging
 *
 * Provides a secondary UDP socket for message redundancy and a
 * deduplication ring buffer to discard duplicate sequence numbers
 * received on either path.
 */

#ifndef CEELL_MSG_REDUNDANCY_H
#define CEELL_MSG_REDUNDANCY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "osal_socket.h"

/** Size of the deduplication ring buffer */
#define CEELL_MSG_DEDUP_RING_SIZE 32

/**
 * Initialize the redundant messaging subsystem.
 * Creates a secondary UDP socket bound to CONFIG_CEELL_MSG_PORT_SECONDARY.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_msg_redundancy_init(void);

/**
 * Send a message on both primary and secondary sockets.
 *
 * @param primary_sock  Primary socket fd (from messaging subsystem)
 * @param buf           Message buffer to send
 * @param len           Length of message
 * @param dest          Destination address
 * @param addrlen       Size of destination address
 * @return 0 on success, negative errno if both sends fail
 */
int ceell_msg_redundant_send(int primary_sock, const void *buf, size_t len,
			     const struct sockaddr *dest, socklen_t addrlen);

/**
 * Check whether a sequence number has already been seen.
 * If the sequence number is new, it is recorded in the ring buffer.
 *
 * @param seq  Sequence number to check
 * @return true if this is a duplicate (already seen), false if new
 */
bool ceell_msg_dedup_check(uint32_t seq);

/**
 * Get the secondary socket file descriptor.
 *
 * @return secondary socket fd, or -1 if not initialized
 */
int ceell_msg_redundancy_get_sock(void);

#endif /* CEELL_MSG_REDUNDANCY_H */
