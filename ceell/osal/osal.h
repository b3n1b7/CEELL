/*
 * CEELL OS Abstraction Layer — Master Include
 *
 * All middleware code includes this header (or specific osal_*.h headers)
 * instead of RTOS-specific headers. This enables portability to
 * SafeRTOS, QNX, or any future certified RTOS.
 *
 * REQ-OSAL-001 — OSAL isolation boundary
 */

#ifndef CEELL_OSAL_H
#define CEELL_OSAL_H

/* Platform detection */
#if defined(__ZEPHYR__)
#define CEELL_OSAL_ZEPHYR 1
#else
#error "No OSAL backend selected. Build with Zephyr or add a new backend."
#endif

/* Common utility macros */
#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* Include all OSAL sub-headers */
#include "osal_thread.h"
#include "osal_sync.h"
#include "osal_time.h"
#include "osal_socket.h"
#include "osal_log.h"
#include "osal_flash.h"
#include "osal_atomic.h"

#endif /* CEELL_OSAL_H */
