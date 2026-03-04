/*
 * CEELL OSAL — Flash Storage
 *
 * Wraps flash area read/write/erase and stream flash operations.
 */

#ifndef CEELL_OSAL_FLASH_H
#define CEELL_OSAL_FLASH_H

#include <stddef.h>
#include <stdint.h>

#if defined(CONFIG_ZEPHYR)
#include <zephyr/storage/flash_map.h>

typedef const struct flash_area ceell_flash_area_t;

#define CEELL_FIXED_PARTITION_ID(label) FIXED_PARTITION_ID(label)

#endif /* CONFIG_ZEPHYR */

/**
 * Open a flash area by partition ID.
 */
int ceell_flash_open(uint8_t id, ceell_flash_area_t **fa);

/**
 * Close a previously opened flash area.
 */
void ceell_flash_close(ceell_flash_area_t *fa);

/**
 * Read data from a flash area.
 */
int ceell_flash_read(ceell_flash_area_t *fa, uint32_t off,
		     void *dst, size_t len);

/**
 * Write data to a flash area.
 */
int ceell_flash_write(ceell_flash_area_t *fa, uint32_t off,
		      const void *src, size_t len);

/**
 * Erase a region of a flash area.
 */
int ceell_flash_erase(ceell_flash_area_t *fa, uint32_t off, size_t len);

/**
 * Get the size of a flash area.
 */
size_t ceell_flash_area_size(ceell_flash_area_t *fa);

/**
 * Get the device and offset of a flash area (for stream flash init).
 */
const void *ceell_flash_area_device(ceell_flash_area_t *fa);
uint32_t ceell_flash_area_offset(ceell_flash_area_t *fa);

#endif /* CEELL_OSAL_FLASH_H */
