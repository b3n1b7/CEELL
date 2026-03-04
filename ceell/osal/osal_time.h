/*
 * CEELL OSAL — Time (uptime, sleep, cycle counter, timeout type)
 * REQ-OSAL-004 — Time abstraction API
 */

#ifndef CEELL_OSAL_TIME_H
#define CEELL_OSAL_TIME_H

#include <stdint.h>

/* Platform-specific timeout type and macros */
#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>

typedef k_timeout_t ceell_timeout_t;

#define CEELL_MSEC(ms)     K_MSEC(ms)
#define CEELL_SECONDS(s)   K_SECONDS(s)
#define CEELL_FOREVER      K_FOREVER
#define CEELL_NO_WAIT      K_NO_WAIT

#endif /* __ZEPHYR__ */

/**
 * Get system uptime in milliseconds.
 */
int64_t ceell_uptime_get(void);

/**
 * Sleep for the specified duration.
 *
 * @param timeout Duration to sleep
 */
void ceell_sleep(ceell_timeout_t timeout);

/**
 * Get current hardware cycle count (for timing measurements).
 */
uint32_t ceell_cycle_get(void);

#endif /* CEELL_OSAL_TIME_H */
