#ifndef CEELL_DISCOVERY_H
#define CEELL_DISCOVERY_H

#include <stdint.h>

struct ceell_peer {
	uint32_t node_id;
	char role[32];
	char name[64];
	char ip[16];
	char services[128];
	int64_t last_seen;  /* uptime ms when last heard */
};

/**
 * Initialize the discovery subsystem. Creates socket, joins multicast group.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_discovery_init(void);

/**
 * Start TX and RX threads for discovery.
 */
void ceell_discovery_start(void);

/**
 * Get current count of known peers (excludes self).
 */
int ceell_discovery_peer_count(void);

/**
 * Get a copy of the peer table.
 *
 * @param out Array to fill
 * @param max_peers Size of out array
 * @return Number of peers copied
 */
int ceell_discovery_get_peers(struct ceell_peer *out, int max_peers);

/**
 * Expire peers not seen within the configured timeout.
 *
 * @return Number of peers expired
 */
int ceell_discovery_expire_peers(void);

#endif /* CEELL_DISCOVERY_H */
