#ifndef CEELL_MESSAGING_H
#define CEELL_MESSAGING_H

#include <zephyr/kernel.h>

/** Response message received from a service request */
struct ceell_msg {
	int status;
	char payload[128];
};

/**
 * Initialize the messaging subsystem. Opens UDP socket on the messaging port,
 * starts the RX thread for incoming requests.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_messaging_init(void);

/**
 * Send a service request to a remote node and wait for a response.
 * Resolves the service name to a peer IP via service discovery,
 * sends a JSON request, and blocks until response or timeout.
 *
 * @param service  Target service name (e.g. "lighting")
 * @param payload  Request payload string
 * @param rsp      Output: response message (status + payload)
 * @param timeout  How long to wait for response
 * @return 0 on success, -ENOENT if service not found, -ETIMEDOUT on timeout,
 *         negative errno on other errors
 */
int ceell_msg_send(const char *service, const char *payload,
		   struct ceell_msg *rsp, k_timeout_t timeout);

#endif /* CEELL_MESSAGING_H */
