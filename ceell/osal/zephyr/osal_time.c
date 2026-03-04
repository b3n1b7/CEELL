/*
 * CEELL OSAL — Zephyr Time Implementation
 */

#include "../osal_time.h"
#include <zephyr/kernel.h>

int64_t ceell_uptime_get(void)
{
	return k_uptime_get();
}

void ceell_sleep(ceell_timeout_t timeout)
{
	k_sleep(timeout);
}

uint32_t ceell_cycle_get(void)
{
	return k_cycle_get_32();
}
