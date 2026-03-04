#ifndef CEELL_MESSAGING_H
#define CEELL_MESSAGING_H

#include "osal_time.h"
#include "msg_priority.h"

#include <stdint.h>
#include <stdbool.h>

/** Response message received from a service request */
struct ceell_msg {
	int status;
	char payload[128];
	enum ceell_msg_priority priority;
	uint16_t deadline_ms;     /* 0 = no deadline */
	int64_t  send_time_ms;    /* uptime when message was sent (for deadline) */
};

/**
 * Initialize the messaging subsystem. Opens UDP socket on the messaging port,
 * starts the dispatcher thread and per-priority handler threads.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_messaging_init(void);

/**
 * Send a service request with explicit priority and wait for a response.
 * Resolves the service name to a peer IP via service discovery,
 * sends a JSON request, and blocks until response or timeout.
 *
 * @param service     Target service name (e.g. "braking")
 * @param payload     Request payload string
 * @param rsp         Output: response message (status + payload)
 * @param timeout     How long to wait for response
 * @param priority    Message priority level
 * @param deadline_ms Deadline in milliseconds (0 = no deadline)
 * @return 0 on success, -ENOENT if service not found, -ETIMEDOUT on timeout,
 *         negative errno on other errors
 */
int ceell_msg_send_pri(const char *service, const char *payload,
		       struct ceell_msg *rsp, ceell_timeout_t timeout,
		       enum ceell_msg_priority priority,
		       uint16_t deadline_ms);

/**
 * Send a service request with NORMAL priority and no deadline.
 * Convenience wrapper around ceell_msg_send_pri().
 *
 * @param service  Target service name (e.g. "lighting")
 * @param payload  Request payload string
 * @param rsp      Output: response message (status + payload)
 * @param timeout  How long to wait for response
 * @return 0 on success, -ENOENT if service not found, -ETIMEDOUT on timeout,
 *         negative errno on other errors
 */
int ceell_msg_send(const char *service, const char *payload,
		   struct ceell_msg *rsp, ceell_timeout_t timeout);

#endif /* CEELL_MESSAGING_H */
