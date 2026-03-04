#ifndef CEELL_OTA_CLIENT_H
#define CEELL_OTA_CLIENT_H

#include <stddef.h>

/**
 * Initialize the OTA subsystem. Starts the OTA polling thread.
 * Must be called after networking and messaging are initialized.
 *
 * If CONFIG_CEELL_OTA_AUTO_CONFIRM is enabled and the firmware booted
 * from a test swap, the current image is confirmed immediately.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_ota_init(void);

/**
 * Trigger an immediate OTA check (bypasses the periodic timer).
 * Can be called from any thread context.
 */
void ceell_ota_trigger_check(void);

/**
 * OTA service handler for the CEELL messaging system.
 * Supports commands: "status", "version", "check".
 */
int ceell_ota_service_handler(const char *payload, char *rsp_payload,
			      size_t rsp_len);

#endif /* CEELL_OTA_CLIENT_H */
