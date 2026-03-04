/*
 * CEELL OSAL — Zephyr Flash Implementation
 */

#include "../osal_flash.h"
#include <zephyr/storage/flash_map.h>

int ceell_flash_open(uint8_t id, ceell_flash_area_t **fa)
{
	return flash_area_open(id, (const struct flash_area **)fa);
}

void ceell_flash_close(ceell_flash_area_t *fa)
{
	flash_area_close(fa);
}

int ceell_flash_read(ceell_flash_area_t *fa, uint32_t off,
		     void *dst, size_t len)
{
	return flash_area_read(fa, off, dst, len);
}

int ceell_flash_write(ceell_flash_area_t *fa, uint32_t off,
		      const void *src, size_t len)
{
	return flash_area_write(fa, off, src, len);
}

int ceell_flash_erase(ceell_flash_area_t *fa, uint32_t off, size_t len)
{
	return flash_area_erase(fa, off, len);
}

size_t ceell_flash_area_size(ceell_flash_area_t *fa)
{
	return fa->fa_size;
}

const void *ceell_flash_area_device(ceell_flash_area_t *fa)
{
	return (const void *)fa->fa_dev;
}

uint32_t ceell_flash_area_offset(ceell_flash_area_t *fa)
{
	return fa->fa_off;
}
