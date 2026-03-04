/*
 * CEELL OSAL — Threading
 * REQ-OSAL-002 — Thread abstraction API
 */

#ifndef CEELL_OSAL_THREAD_H
#define CEELL_OSAL_THREAD_H

#include <stdint.h>

/* Platform-specific type definitions */
#if defined(CONFIG_ZEPHYR)
#include <zephyr/kernel.h>

typedef struct k_thread ceell_thread_t;

#define CEELL_THREAD_STACK_DEFINE(name, size) \
	K_THREAD_STACK_DEFINE(name, size)

#define CEELL_THREAD_STACK_SIZEOF(stack) \
	K_THREAD_STACK_SIZEOF(stack)

#endif /* CONFIG_ZEPHYR */

/** Thread entry function signature */
typedef void (*ceell_thread_entry_t)(void *p1, void *p2, void *p3);

/**
 * Create and start a new thread.
 *
 * @param thread   Thread control block (caller-allocated)
 * @param stack    Stack buffer (from CEELL_THREAD_STACK_DEFINE)
 * @param stack_sz Stack size (from CEELL_THREAD_STACK_SIZEOF)
 * @param entry    Thread entry function
 * @param p1       First argument to entry
 * @param p2       Second argument to entry
 * @param p3       Third argument to entry
 * @param priority Thread priority (lower = higher priority)
 * @return 0 on success, negative errno on failure
 */
int ceell_thread_create(ceell_thread_t *thread, void *stack,
			size_t stack_sz, ceell_thread_entry_t entry,
			void *p1, void *p2, void *p3, int priority);

/**
 * Abort (terminate) a thread.
 *
 * @param thread Thread to abort
 */
void ceell_thread_abort(ceell_thread_t *thread);

/**
 * Set the priority of a thread.
 *
 * @param thread   Thread to modify
 * @param priority New priority
 * @return 0 on success, negative errno on failure
 */
int ceell_thread_priority_set(ceell_thread_t *thread, int priority);

/**
 * Set the name of a thread (for debug/logging).
 *
 * @param thread Thread to name
 * @param name   Name string (not copied — must remain valid)
 * @return 0 on success, negative errno on failure
 */
int ceell_thread_name_set(ceell_thread_t *thread, const char *name);

#endif /* CEELL_OSAL_THREAD_H */
