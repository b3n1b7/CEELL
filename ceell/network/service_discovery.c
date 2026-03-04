/* REQ-NET-002 */

#include "service_discovery.h"
#include "discovery.h"

#include <string.h>
#include <stdbool.h>
#include <errno.h>

/**
 * Check if a comma-separated list contains a specific service name.
 * Exact match on each token (not substring).
 */
static bool csv_contains(const char *csv, const char *name)
{
	size_t name_len = strlen(name);
	const char *p = csv;

	while (*p) {
		const char *comma = strchr(p, ',');
		size_t token_len;

		if (comma) {
			token_len = comma - p;
		} else {
			token_len = strlen(p);
		}

		if (token_len == name_len && memcmp(p, name, name_len) == 0) {
			return true;
		}

		if (comma) {
			p = comma + 1;
		} else {
			break;
		}
	}

	return false;
}

int ceell_service_find_peer(const char *service_name, char *ip_out)
{
	struct ceell_peer peers[CONFIG_CEELL_DISCOVERY_MAX_PEERS];
	int count;

	if (!service_name || !ip_out) {
		return -EINVAL;
	}

	count = ceell_discovery_get_peers(peers, CONFIG_CEELL_DISCOVERY_MAX_PEERS);

	for (int i = 0; i < count; i++) {
		if (csv_contains(peers[i].services, service_name)) {
			strncpy(ip_out, peers[i].ip, 15);
			ip_out[15] = '\0';
			return 0;
		}
	}

	return -ENOENT;
}
