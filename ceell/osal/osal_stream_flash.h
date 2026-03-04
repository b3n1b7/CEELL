/*
 * CEELL OSAL — Stream Flash
 *
 * Wraps buffered flash write operations for firmware downloads.
 * On Zephyr, wraps <zephyr/storage/stream_flash.h>.
 */

#ifndef CEELL_OSAL_STREAM_FLASH_H
#define CEELL_OSAL_STREAM_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__ZEPHYR__)
#include <zephyr/storage/stream_flash.h>

typedef struct stream_flash_ctx ceell_stream_flash_ctx_t;

#endif /* __ZEPHYR__ */

/**
 * Initialize a stream flash context for buffered writes.
 *
 * @param ctx      Stream flash context (caller-allocated)
 * @param fdev     Flash device
 * @param buf      Write buffer (caller-allocated)
 * @param buf_len  Write buffer size
 * @param offset   Start offset in flash
 * @param size     Total size available
 * @param cb       Optional progress callback (NULL for none)
 * @return 0 on success, negative errno on failure
 */
int ceell_stream_flash_init(ceell_stream_flash_ctx_t *ctx,
			    const void *fdev, uint8_t *buf, size_t buf_len,
			    size_t offset, size_t size, void *cb);

/**
 * Write data to stream flash (buffered).
 *
 * @param ctx   Stream flash context
 * @param data  Data to write (NULL to flush)
 * @param len   Data length (0 when flushing)
 * @param flush If true, flush remaining buffer to flash
 * @return 0 on success, negative errno on failure
 */
int ceell_stream_flash_write(ceell_stream_flash_ctx_t *ctx,
			     const uint8_t *data, size_t len, bool flush);

#endif /* CEELL_OSAL_STREAM_FLASH_H */
