/*
 * CEELL OSAL — Logging
 *
 * Provides CEELL_LOG_INF/ERR/WRN/DBG macros and a printk equivalent.
 * On Zephyr, wraps the LOG_* macros and printk().
 */

#ifndef CEELL_OSAL_LOG_H
#define CEELL_OSAL_LOG_H

#if defined(CONFIG_ZEPHYR)
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>  /* for printk */

#define CEELL_LOG_MODULE_REGISTER(name, level) \
	LOG_MODULE_REGISTER(name, level)

#define CEELL_LOG_MODULE_DECLARE(name) \
	LOG_MODULE_DECLARE(name)

#define CEELL_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define CEELL_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define CEELL_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define CEELL_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define CEELL_LOG_LEVEL_INF LOG_LEVEL_INF
#define CEELL_LOG_LEVEL_ERR LOG_LEVEL_ERR
#define CEELL_LOG_LEVEL_WRN LOG_LEVEL_WRN
#define CEELL_LOG_LEVEL_DBG LOG_LEVEL_DBG

#define ceell_printk printk

#endif /* CONFIG_ZEPHYR */

#endif /* CEELL_OSAL_LOG_H */
