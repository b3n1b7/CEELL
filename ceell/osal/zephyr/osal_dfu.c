/*
 * CEELL OSAL — Zephyr DFU/Bootloader Implementation
 */

#include "../osal_dfu.h"
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>

bool ceell_boot_is_confirmed(void)
{
	return boot_is_img_confirmed();
}

int ceell_boot_confirm(void)
{
	return boot_write_img_confirmed();
}

int ceell_boot_request_upgrade(void)
{
	return boot_request_upgrade(BOOT_UPGRADE_TEST);
}

void ceell_sys_reboot(void)
{
	sys_reboot(SYS_REBOOT_COLD);
}
