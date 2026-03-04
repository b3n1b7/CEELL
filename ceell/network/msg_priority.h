/*
 * CEELL Safety-Critical Messaging — Priority Levels
 *
 * Defines message priority classes for the 48V vehicle domain.
 * Lower enum value = higher urgency.
 */

#ifndef CEELL_MSG_PRIORITY_H
#define CEELL_MSG_PRIORITY_H

#include <stdbool.h>

/**
 * Message priority levels. Lower value = higher priority.
 *
 * CRITICAL: safety-critical commands (brake, steer) — lowest latency
 * NORMAL:   standard service requests and responses
 * BULK:     diagnostics, OTA status, telemetry — best effort
 */
enum ceell_msg_priority {
	CEELL_MSG_CRITICAL = 0,
	CEELL_MSG_NORMAL   = 1,
	CEELL_MSG_BULK     = 2,
	CEELL_MSG_PRIORITY_COUNT
};

/**
 * Check whether a priority value is valid.
 *
 * @param pri Priority value to check
 * @return true if valid, false otherwise
 */
static inline bool ceell_msg_priority_valid(int pri)
{
	return (pri >= CEELL_MSG_CRITICAL && pri < CEELL_MSG_PRIORITY_COUNT);
}

/**
 * Return a human-readable label for a priority level.
 */
static inline const char *ceell_msg_priority_name(enum ceell_msg_priority pri)
{
	switch (pri) {
	case CEELL_MSG_CRITICAL: return "CRITICAL";
	case CEELL_MSG_NORMAL:   return "NORMAL";
	case CEELL_MSG_BULK:     return "BULK";
	default:                 return "UNKNOWN";
	}
}

#endif /* CEELL_MSG_PRIORITY_H */
