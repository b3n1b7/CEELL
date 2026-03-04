/*
 * CEELL OSAL — Zephyr Synchronization Implementation
 */

#include "../osal_sync.h"
#include <zephyr/kernel.h>

/* ── Mutex ─────────────────────────────────────────────────────── */

int ceell_mutex_init(ceell_mutex_t *mutex)
{
	return k_mutex_init(mutex);
}

int ceell_mutex_lock(ceell_mutex_t *mutex)
{
	return k_mutex_lock(mutex, K_FOREVER);
}

int ceell_mutex_unlock(ceell_mutex_t *mutex)
{
	return k_mutex_unlock(mutex);
}

/* ── Semaphore ─────────────────────────────────────────────────── */

int ceell_sem_init(ceell_sem_t *sem, unsigned int initial, unsigned int limit)
{
	return k_sem_init(sem, initial, limit);
}

int ceell_sem_give(ceell_sem_t *sem)
{
	k_sem_give(sem);
	return 0;
}

int ceell_sem_take(ceell_sem_t *sem, ceell_timeout_t timeout)
{
	return k_sem_take(sem, timeout);
}

int ceell_sem_reset(ceell_sem_t *sem)
{
	k_sem_reset(sem);
	return 0;
}

/* ── Message Queue ─────────────────────────────────────────────── */

int ceell_msgq_init(ceell_msgq_t *msgq, void *buf, size_t msg_size,
		    uint32_t max_msgs)
{
	k_msgq_init(msgq, (char *)buf, msg_size, max_msgs);
	return 0;
}

int ceell_msgq_put(ceell_msgq_t *msgq, const void *data,
		   ceell_timeout_t timeout)
{
	return k_msgq_put(msgq, data, timeout);
}

int ceell_msgq_get(ceell_msgq_t *msgq, void *data, ceell_timeout_t timeout)
{
	return k_msgq_get(msgq, data, timeout);
}
