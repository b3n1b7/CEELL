#ifndef CEELL_OTA_VERSION_H
#define CEELL_OTA_VERSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ceell_ota_ver {
	uint8_t major;
	uint8_t minor;
	uint16_t patch;
};

/**
 * Parse a "major.minor.patch" version string.
 *
 * @param str   Version string (e.g. "0.2.0")
 * @param ver   Output parsed version
 * @return 0 on success, -EINVAL on parse error
 */
int ceell_ota_ver_parse(const char *str, struct ceell_ota_ver *ver);

/**
 * Compare two versions.
 *
 * @return negative if a < b, 0 if equal, positive if a > b
 */
int ceell_ota_ver_cmp(const struct ceell_ota_ver *a,
		      const struct ceell_ota_ver *b);

/**
 * Format a version into a string buffer.
 *
 * @param ver   Version to format
 * @param buf   Output buffer
 * @param len   Buffer length
 * @return number of characters written (excluding NUL)
 */
int ceell_ota_ver_to_str(const struct ceell_ota_ver *ver, char *buf,
			 size_t len);

/**
 * Get the running firmware version (from VERSION file at build time).
 */
void ceell_ota_ver_current(struct ceell_ota_ver *ver);

#endif /* CEELL_OTA_VERSION_H */
