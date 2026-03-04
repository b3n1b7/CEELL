/*
 * CEELL OSAL — Synchronization (mutex, semaphore, message queue)
 * /* REQ-OSAL-003 */ Synchronization abstraction API
 */

#ifndef CEELL_OSAL_SYNC_H
#define CEELL_OSAL_SYNC_H

#include <stddef.h>
#include <stdint.h>
#include "osal_time.h"

/* Platform-specific type definitions */
#if defined(CONFIG_ZEPHYR)
#include <zephyr/kernel.h>

typedef struct k_mutex ceell_mutex_t;
typedef struct k_sem   ceell_sem_t;
typedef struct k_msgq  ceell_msgq_t;

#endif /* CONFIG_ZEPHYR */

/* ── Mutex ─────────────────────────────────────────────────────── */

int ceell_mutex_init(ceell_mutex_t *mutex);
int ceell_mutex_lock(ceell_mutex_t *mutex);
int ceell_mutex_unlock(ceell_mutex_t *mutex);

/* ── Semaphore ─────────────────────────────────────────────────── */

int ceell_sem_init(ceell_sem_t *sem, unsigned int initial, unsigned int limit);
int ceell_sem_give(ceell_sem_t *sem);
int ceell_sem_take(ceell_sem_t *sem, ceell_timeout_t timeout);
int ceell_sem_reset(ceell_sem_t *sem);

/* ── Message Queue ─────────────────────────────────────────────── */

/**
 * Initialize a message queue.
 *
 * @param msgq     Message queue control block (caller-allocated)
 * @param buf      Buffer for queue storage (caller-allocated)
 * @param msg_size Size of each message in bytes
 * @param max_msgs Maximum number of messages the queue can hold
 * @return 0 on success, negative errno on failure
 */
int ceell_msgq_init(ceell_msgq_t *msgq, void *buf, size_t msg_size,
		    uint32_t max_msgs);

/**
 * Put a message into the queue.
 *
 * @param msgq    Message queue
 * @param data    Pointer to message data (msg_size bytes)
 * @param timeout How long to wait if queue is full
 * @return 0 on success, -EAGAIN on timeout, negative errno on failure
 */
int ceell_msgq_put(ceell_msgq_t *msgq, const void *data,
		   ceell_timeout_t timeout);

/**
 * Get a message from the queue.
 *
 * @param msgq    Message queue
 * @param data    Buffer to receive message (msg_size bytes)
 * @param timeout How long to wait if queue is empty
 * @return 0 on success, -EAGAIN on timeout, negative errno on failure
 */
int ceell_msgq_get(ceell_msgq_t *msgq, void *data, ceell_timeout_t timeout);

#endif /* CEELL_OSAL_SYNC_H */
