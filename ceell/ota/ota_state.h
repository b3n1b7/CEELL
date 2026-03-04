#ifndef CEELL_OTA_STATE_H
#define CEELL_OTA_STATE_H

enum ceell_ota_state {
	CEELL_OTA_IDLE,
	CEELL_OTA_CHECKING,
	CEELL_OTA_DOWNLOADING,
	CEELL_OTA_VALIDATING,
	CEELL_OTA_MARKING,
	CEELL_OTA_REBOOTING,
	CEELL_OTA_ERROR,
};

/**
 * Get the current OTA state (thread-safe).
 */
enum ceell_ota_state ceell_ota_get_state(void);

/**
 * Set the current OTA state (thread-safe).
 */
void ceell_ota_set_state(enum ceell_ota_state state);

/**
 * Get a human-readable name for an OTA state.
 */
const char *ceell_ota_state_str(enum ceell_ota_state state);

#endif /* CEELL_OTA_STATE_H */
