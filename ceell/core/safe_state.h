/*
 * CEELL Safe State Management
 *
 * Provides a mechanism to trigger, query, and clear a system-wide
 * safe state. When triggered, registered callbacks are notified
 * and the active flag is set atomically.
 */

#ifndef CEELL_SAFE_STATE_H
#define CEELL_SAFE_STATE_H

#include <stdint.h>
#include <stdbool.h>

/** Reasons for entering safe state */
enum ceell_safe_reason {
	CEELL_SAFE_NONE                    = 0, /**< No safe state active */
	CEELL_SAFE_CRITICAL_PEER_LOST      = 1, /**< A critical peer was lost */
	CEELL_SAFE_DEADLINE_BUDGET_EXCEEDED = 2, /**< Message deadline exceeded */
	CEELL_SAFE_COMM_FAILURE            = 3, /**< Communication subsystem failure */
};

/**
 * Callback type for safe state transitions.
 *
 * @param active  true if entering safe state, false if clearing
 * @param reason  The reason code (from enum ceell_safe_reason)
 */
typedef void (*ceell_safe_state_cb_t)(bool active, int reason);

/**
 * Initialize the safe state manager.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_safe_state_init(void);

/**
 * Trigger safe state with a given reason.
 * Logs the reason, sets the atomic flag, and calls registered callbacks.
 *
 * @param reason  One of enum ceell_safe_reason values
 */
void ceell_safe_state_trigger(int reason);

/**
 * Clear the safe state (recovery).
 * Resets the active flag and calls registered callbacks with active=false.
 */
void ceell_safe_state_clear(void);

/**
 * Check if the system is currently in safe state.
 *
 * @return true if safe state is active
 */
bool ceell_safe_state_active(void);

/**
 * Get the reason for the current safe state.
 *
 * @return Reason code, or CEELL_SAFE_NONE if not in safe state
 */
int ceell_safe_state_reason(void);

/**
 * Register a callback for safe state transitions.
 * Only one callback is supported; subsequent calls overwrite.
 *
 * @param cb  Callback function (NULL to unregister)
 */
void ceell_safe_state_register_cb(ceell_safe_state_cb_t cb);

#endif /* CEELL_SAFE_STATE_H */
