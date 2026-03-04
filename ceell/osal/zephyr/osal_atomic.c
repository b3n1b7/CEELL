/*
 * CEELL OSAL — Zephyr Atomic Implementation
 */

#include "../osal_atomic.h"
#include <zephyr/sys/atomic.h>

void ceell_atomic_set(ceell_atomic_t *target, int32_t value)
{
	atomic_set(target, (atomic_val_t)value);
}

int32_t ceell_atomic_get(const ceell_atomic_t *target)
{
	return (int32_t)atomic_get(target);
}

int32_t ceell_atomic_inc(ceell_atomic_t *target)
{
	return (int32_t)atomic_inc(target) + 1;
}

int32_t ceell_atomic_dec(ceell_atomic_t *target)
{
	return (int32_t)atomic_dec(target) - 1;
}
