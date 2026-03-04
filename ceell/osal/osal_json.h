/*
 * CEELL OSAL — JSON Library
 *
 * Wraps the platform's JSON serialization/deserialization.
 * On Zephyr, re-exports <zephyr/data/json.h>.
 */

#ifndef CEELL_OSAL_JSON_H
#define CEELL_OSAL_JSON_H

#if defined(__ZEPHYR__)
#include <zephyr/data/json.h>
#endif

#endif /* CEELL_OSAL_JSON_H */
