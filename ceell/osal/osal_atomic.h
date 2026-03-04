/*
 * CEELL OSAL — Atomic Operations
 */

#ifndef CEELL_OSAL_ATOMIC_H
#define CEELL_OSAL_ATOMIC_H

#include <stdint.h>

#if defined(__ZEPHYR__)
#include <zephyr/sys/atomic.h>

typedef atomic_t ceell_atomic_t;

#define CEELL_ATOMIC_INIT(val) ATOMIC_INIT(val)

#endif /* __ZEPHYR__ */

/**
 * Atomically set a value.
 */
void ceell_atomic_set(ceell_atomic_t *target, int32_t value);

/**
 * Atomically get a value.
 */
int32_t ceell_atomic_get(const ceell_atomic_t *target);

/**
 * Atomically increment and return new value.
 */
int32_t ceell_atomic_inc(ceell_atomic_t *target);

/**
 * Atomically decrement and return new value.
 */
int32_t ceell_atomic_dec(ceell_atomic_t *target);

#endif /* CEELL_OSAL_ATOMIC_H */
