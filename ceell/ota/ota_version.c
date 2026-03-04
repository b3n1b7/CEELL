/* REQ-OTA-002 */

#include "ota_version.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osal_version.h"

int ceell_ota_ver_parse(const char *str, struct ceell_ota_ver *ver)
{
	unsigned long parts[3];
	const char *p = str;
	char *end;

	if (!str || !ver) {
		return -EINVAL;
	}

	for (int i = 0; i < 3; i++) {
		parts[i] = strtoul(p, &end, 10);
		if (end == p) {
			return -EINVAL;
		}
		if (i < 2) {
			if (*end != '.') {
				return -EINVAL;
			}
			p = end + 1;
		}
	}

	if (parts[0] > 255 || parts[1] > 255 || parts[2] > 65535) {
		return -EINVAL;
	}

	ver->major = (uint8_t)parts[0];
	ver->minor = (uint8_t)parts[1];
	ver->patch = (uint16_t)parts[2];
	return 0;
}

int ceell_ota_ver_cmp(const struct ceell_ota_ver *a,
		      const struct ceell_ota_ver *b)
{
	if (a->major != b->major) {
		return (int)a->major - (int)b->major;
	}
	if (a->minor != b->minor) {
		return (int)a->minor - (int)b->minor;
	}
	return (int)a->patch - (int)b->patch;
}

int ceell_ota_ver_to_str(const struct ceell_ota_ver *ver, char *buf,
			 size_t len)
{
	return snprintf(buf, len, "%u.%u.%u", ver->major, ver->minor,
			ver->patch);
}

void ceell_ota_ver_current(struct ceell_ota_ver *ver)
{
	ver->major = CEELL_APP_VERSION_MAJOR;
	ver->minor = CEELL_APP_VERSION_MINOR;
	ver->patch = CEELL_APP_PATCHLEVEL;
}
