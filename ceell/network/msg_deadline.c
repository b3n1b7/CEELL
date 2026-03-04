#include "msg_deadline.h"
#include "messaging.h"
#include "osal.h"

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

int ceell_msg_deadline_remaining(const struct ceell_msg *msg)
{
	if (!msg || msg->deadline_ms == 0) {
		return 0;
	}

	int64_t now = ceell_uptime_get();
	int64_t deadline_abs = msg->send_time_ms + (int64_t)msg->deadline_ms;
	int64_t remaining = deadline_abs - now;

	/* Clamp to int range */
	if (remaining > (int64_t)INT32_MAX) {
		return INT32_MAX;
	}
	if (remaining < (int64_t)INT32_MIN) {
		return INT32_MIN;
	}

	return (int)remaining;
}

bool ceell_msg_deadline_expired(const struct ceell_msg *msg)
{
	if (!msg || msg->deadline_ms == 0) {
		return false;
	}

	int64_t now = ceell_uptime_get();
	int64_t deadline_abs = msg->send_time_ms + (int64_t)msg->deadline_ms;

	return (now >= deadline_abs);
}
