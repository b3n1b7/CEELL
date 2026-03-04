/*
 * CEELL OSAL — DFU / Bootloader Operations
 *
 * Wraps MCUboot / bootloader operations for OTA updates.
 * On Zephyr, wraps <zephyr/dfu/mcuboot.h> and <zephyr/sys/reboot.h>.
 */

#ifndef CEELL_OSAL_DFU_H
#define CEELL_OSAL_DFU_H

#include <stdbool.h>

/**
 * Check if the current firmware image is confirmed.
 */
bool ceell_boot_is_confirmed(void);

/**
 * Confirm (make permanent) the currently running firmware image.
 * @return 0 on success, negative errno on failure
 */
int ceell_boot_confirm(void);

/**
 * Request a firmware upgrade (test swap on next reboot).
 * @return 0 on success, negative errno on failure
 */
int ceell_boot_request_upgrade(void);

/**
 * Trigger a system reboot.
 */
void ceell_sys_reboot(void);

#endif /* CEELL_OSAL_DFU_H */
