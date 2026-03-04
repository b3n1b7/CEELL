/*
 * CEELL OSAL — Zephyr Stream Flash Implementation
 */

#include "../osal_stream_flash.h"
#include <zephyr/device.h>
#include <zephyr/storage/stream_flash.h>

int ceell_stream_flash_init(ceell_stream_flash_ctx_t *ctx,
			    const void *fdev, uint8_t *buf, size_t buf_len,
			    size_t offset, size_t size, void *cb)
{
	return stream_flash_init(ctx, (const struct device *)fdev,
				 buf, buf_len, offset, size,
				 (stream_flash_callback_t)cb);
}

int ceell_stream_flash_write(ceell_stream_flash_ctx_t *ctx,
			     const uint8_t *data, size_t len, bool flush)
{
	return stream_flash_buffered_write(ctx, data, len, flush);
}
