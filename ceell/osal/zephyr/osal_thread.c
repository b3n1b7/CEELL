/*
 * CEELL OSAL — Zephyr Thread Implementation
 */

#include "../osal_thread.h"
#include <zephyr/kernel.h>

int ceell_thread_create(ceell_thread_t *thread, void *stack,
			size_t stack_sz, ceell_thread_entry_t entry,
			void *p1, void *p2, void *p3, int priority)
{
	k_tid_t tid;

	tid = k_thread_create(thread, (k_thread_stack_t *)stack, stack_sz,
			      (k_thread_entry_t)entry, p1, p2, p3,
			      priority, 0, K_NO_WAIT);
	return (tid != NULL) ? 0 : -EAGAIN;
}

void ceell_thread_abort(ceell_thread_t *thread)
{
	k_thread_abort(thread);
}

int ceell_thread_priority_set(ceell_thread_t *thread, int priority)
{
	k_thread_priority_set(thread, priority);
	return 0;
}

int ceell_thread_name_set(ceell_thread_t *thread, const char *name)
{
	return k_thread_name_set(thread, name);
}
