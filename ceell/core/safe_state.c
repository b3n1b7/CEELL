/*
 * CEELL Safe State Management — Implementation
 *
 * Uses an atomic flag for the active state (lock-free reads) and
 * a mutex to serialize trigger/clear operations and callback dispatch.
 */

#include "safe_state.h"
#include "osal.h"

#include <errno.h>

CEELL_LOG_MODULE_REGISTER(ceell_safe_state, CEELL_LOG_LEVEL_INF);

static ceell_atomic_t safe_active = CEELL_ATOMIC_INIT(0);
static ceell_atomic_t safe_reason_val = CEELL_ATOMIC_INIT(0);
static ceell_mutex_t safe_mutex;
static ceell_safe_state_cb_t safe_cb;
static bool safe_initialized;

static const char *reason_str(int reason)
{
	switch (reason) {
	case CEELL_SAFE_CRITICAL_PEER_LOST:
		return "CRITICAL_PEER_LOST";
	case CEELL_SAFE_DEADLINE_BUDGET_EXCEEDED:
		return "DEADLINE_BUDGET_EXCEEDED";
	case CEELL_SAFE_COMM_FAILURE:
		return "COMM_FAILURE";
	default:
		return "UNKNOWN";
	}
}

int ceell_safe_state_init(void)
{
	int ret;

	ret = ceell_mutex_init(&safe_mutex);
	if (ret < 0) {
		return ret;
	}

	ceell_atomic_set(&safe_active, 0);
	ceell_atomic_set(&safe_reason_val, CEELL_SAFE_NONE);
	safe_cb = NULL;
	safe_initialized = true;

	CEELL_LOG_INF("Safe state manager initialized");
	return 0;
}

void ceell_safe_state_trigger(int reason)
{
	if (!safe_initialized) {
		return;
	}

	ceell_mutex_lock(&safe_mutex);

	/* Only log/callback on transition from inactive to active */
	if (ceell_atomic_get(&safe_active) == 0) {
		CEELL_LOG_ERR("SAFE STATE TRIGGERED: reason=%s (%d)",
			      reason_str(reason), reason);

		ceell_atomic_set(&safe_reason_val, reason);
		ceell_atomic_set(&safe_active, 1);

		if (safe_cb) {
			safe_cb(true, reason);
		}
	} else {
		/* Already in safe state — update reason if different */
		int current = ceell_atomic_get(&safe_reason_val);

		if (current != reason) {
			CEELL_LOG_ERR("SAFE STATE reason update: %s -> %s",
				      reason_str(current), reason_str(reason));
			ceell_atomic_set(&safe_reason_val, reason);
		}
	}

	ceell_mutex_unlock(&safe_mutex);
}

void ceell_safe_state_clear(void)
{
	if (!safe_initialized) {
		return;
	}

	ceell_mutex_lock(&safe_mutex);

	if (ceell_atomic_get(&safe_active) != 0) {
		int reason = ceell_atomic_get(&safe_reason_val);

		CEELL_LOG_INF("SAFE STATE CLEARED (was: %s)", reason_str(reason));

		ceell_atomic_set(&safe_active, 0);
		ceell_atomic_set(&safe_reason_val, CEELL_SAFE_NONE);

		if (safe_cb) {
			safe_cb(false, CEELL_SAFE_NONE);
		}
	}

	ceell_mutex_unlock(&safe_mutex);
}

bool ceell_safe_state_active(void)
{
	return ceell_atomic_get(&safe_active) != 0;
}

int ceell_safe_state_reason(void)
{
	if (!ceell_safe_state_active()) {
		return CEELL_SAFE_NONE;
	}
	return ceell_atomic_get(&safe_reason_val);
}

void ceell_safe_state_register_cb(ceell_safe_state_cb_t cb)
{
	if (!safe_initialized) {
		return;
	}

	ceell_mutex_lock(&safe_mutex);
	safe_cb = cb;
	ceell_mutex_unlock(&safe_mutex);
}
