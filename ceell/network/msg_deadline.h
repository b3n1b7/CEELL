/*
 * CEELL Safety-Critical Messaging — Deadline Monitoring
 *
 * Provides deadline calculation and expiry checks for safety-critical
 * messages in the 48V vehicle domain.
 */

#ifndef CEELL_MSG_DEADLINE_H
#define CEELL_MSG_DEADLINE_H

#include <stdbool.h>

struct ceell_msg;

/**
 * Calculate milliseconds remaining until a message's deadline expires.
 *
 * Uses the message's send_time_ms + deadline_ms to determine the absolute
 * deadline, then subtracts the current uptime.
 *
 * @param msg Message to check (must have send_time_ms and deadline_ms set)
 * @return Milliseconds remaining (positive), 0 if no deadline configured,
 *         or negative if the deadline has already expired
 */
int ceell_msg_deadline_remaining(const struct ceell_msg *msg);

/**
 * Check whether a message's deadline has expired.
 *
 * @param msg Message to check
 * @return true if the deadline has passed, false if still within deadline
 *         or if no deadline is configured (deadline_ms == 0)
 */
bool ceell_msg_deadline_expired(const struct ceell_msg *msg);

#endif /* CEELL_MSG_DEADLINE_H */
