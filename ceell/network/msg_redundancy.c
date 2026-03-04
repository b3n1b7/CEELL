/*
 * CEELL Dual-Path Redundant Messaging — Implementation
 *
 * Creates a secondary UDP socket and provides a deduplication ring buffer
 * so that messages sent on both paths are only processed once.
 */

#include "msg_redundancy.h"
#include "osal.h"

#include <string.h>
#include <errno.h>

CEELL_LOG_MODULE_REGISTER(ceell_msg_redundancy, CEELL_LOG_LEVEL_INF);

/* Secondary socket for redundant path */
static int secondary_sock = -1;

/* Deduplication ring buffer */
static uint32_t dedup_ring[CEELL_MSG_DEDUP_RING_SIZE];
static int dedup_head;
static int dedup_count;
static ceell_mutex_t dedup_mutex;

int ceell_msg_redundancy_init(void)
{
	struct sockaddr_in bind_addr;
	int ret;

	ret = ceell_mutex_init(&dedup_mutex);
	if (ret < 0) {
		CEELL_LOG_ERR("dedup mutex init failed (%d)", ret);
		return ret;
	}

	/* Clear the dedup ring */
	memset(dedup_ring, 0, sizeof(dedup_ring));
	dedup_head = 0;
	dedup_count = 0;

	/* Create secondary UDP socket */
	secondary_sock = ceell_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (secondary_sock < 0) {
		CEELL_LOG_ERR("secondary socket failed (%d)", secondary_sock);
		return secondary_sock;
	}

	int optval = 1;

	ceell_setsockopt(secondary_sock, SOL_SOCKET, SO_REUSEADDR,
			 &optval, sizeof(optval));

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(CONFIG_CEELL_MSG_PORT_SECONDARY);
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = ceell_bind(secondary_sock, (struct sockaddr *)&bind_addr,
			 sizeof(bind_addr));
	if (ret < 0) {
		CEELL_LOG_ERR("secondary bind failed (%d)", ret);
		ceell_close(secondary_sock);
		secondary_sock = -1;
		return ret;
	}

	CEELL_LOG_INF("Redundant messaging initialized on port %d",
		      CONFIG_CEELL_MSG_PORT_SECONDARY);
	return 0;
}

int ceell_msg_redundant_send(int primary_sock, const void *buf, size_t len,
			     const struct sockaddr *dest, socklen_t addrlen)
{
	int ret_primary;
	int ret_secondary;

	/* Send on primary path */
	ret_primary = ceell_sendto(primary_sock, buf, len, 0, dest, addrlen);

	/* Send on secondary path */
	if (secondary_sock >= 0) {
		/* Adjust destination port to secondary */
		struct sockaddr_in sec_dest;

		memcpy(&sec_dest, dest, sizeof(sec_dest));
		sec_dest.sin_port = htons(CONFIG_CEELL_MSG_PORT_SECONDARY);

		ret_secondary = ceell_sendto(secondary_sock, buf, len, 0,
					     (const struct sockaddr *)&sec_dest,
					     sizeof(sec_dest));
	} else {
		ret_secondary = -ENODEV;
	}

	/* Success if at least one path succeeded */
	if (ret_primary >= 0 || ret_secondary >= 0) {
		return 0;
	}

	/* Both failed — return primary error */
	return ret_primary;
}

bool ceell_msg_dedup_check(uint32_t seq)
{
	ceell_mutex_lock(&dedup_mutex);

	/* Search the ring buffer for this sequence number */
	int search_count = (dedup_count < CEELL_MSG_DEDUP_RING_SIZE)
			   ? dedup_count : CEELL_MSG_DEDUP_RING_SIZE;

	for (int i = 0; i < search_count; i++) {
		int idx = (dedup_head - 1 - i + CEELL_MSG_DEDUP_RING_SIZE)
			  % CEELL_MSG_DEDUP_RING_SIZE;

		if (dedup_ring[idx] == seq) {
			ceell_mutex_unlock(&dedup_mutex);
			return true; /* duplicate */
		}
	}

	/* New sequence — record it */
	dedup_ring[dedup_head] = seq;
	dedup_head = (dedup_head + 1) % CEELL_MSG_DEDUP_RING_SIZE;
	if (dedup_count < CEELL_MSG_DEDUP_RING_SIZE) {
		dedup_count++;
	}

	ceell_mutex_unlock(&dedup_mutex);
	return false; /* not a duplicate */
}

int ceell_msg_redundancy_get_sock(void)
{
	return secondary_sock;
}
