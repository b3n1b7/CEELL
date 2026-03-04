/* REQ-OTA-001 */

#include "ota_state.h"

#include "osal_atomic.h"

static ceell_atomic_t current_state = CEELL_ATOMIC_INIT(CEELL_OTA_IDLE);

enum ceell_ota_state ceell_ota_get_state(void)
{
	return (enum ceell_ota_state)ceell_atomic_get(&current_state);
}

void ceell_ota_set_state(enum ceell_ota_state state)
{
	ceell_atomic_set(&current_state, (int32_t)state);
}

const char *ceell_ota_state_str(enum ceell_ota_state state)
{
	switch (state) {
	case CEELL_OTA_IDLE:
		return "idle";
	case CEELL_OTA_CHECKING:
		return "checking";
	case CEELL_OTA_DOWNLOADING:
		return "downloading";
	case CEELL_OTA_VALIDATING:
		return "validating";
	case CEELL_OTA_MARKING:
		return "marking";
	case CEELL_OTA_REBOOTING:
		return "rebooting";
	case CEELL_OTA_ERROR:
		return "error";
	default:
		return "unknown";
	}
}
