/*
 * CEELL Health Monitor — Implementation
 *
 * Tracks peer health by comparing discovery peer table timestamps
 * against the current uptime. Transitions peers through
 * HEALTHY -> DEGRADED -> CRITICAL -> LOST based on missed intervals.
 */

#include "health_monitor.h"
#include "discovery.h"
#include "osal.h"

#include <string.h>
#include <errno.h>

CEELL_LOG_MODULE_REGISTER(ceell_health, CEELL_LOG_LEVEL_INF);

#ifndef CONFIG_CEELL_DISCOVERY_MAX_PEERS
#define CONFIG_CEELL_DISCOVERY_MAX_PEERS 8
#endif

#ifndef CONFIG_CEELL_DISCOVERY_INTERVAL_MS
#define CONFIG_CEELL_DISCOVERY_INTERVAL_MS 1000
#endif

#ifndef CONFIG_CEELL_PEER_TIMEOUT_MS
#define CONFIG_CEELL_PEER_TIMEOUT_MS 5000
#endif

/** Health table — one entry per potential peer */
static struct ceell_peer_health health_table[CONFIG_CEELL_DISCOVERY_MAX_PEERS];
static ceell_mutex_t health_mutex;
static ceell_health_cb_t health_cb;
static bool health_initialized;

/**
 * Find or allocate a slot in the health table for a given node_id.
 * Must be called with health_mutex held.
 */
static struct ceell_peer_health *find_or_alloc(uint32_t node_id)
{
	struct ceell_peer_health *free_slot = NULL;

	for (int i = 0; i < CONFIG_CEELL_DISCOVERY_MAX_PEERS; i++) {
		if (health_table[i].active && health_table[i].node_id == node_id) {
			return &health_table[i];
		}
		if (!health_table[i].active && !free_slot) {
			free_slot = &health_table[i];
		}
	}

	if (free_slot) {
		memset(free_slot, 0, sizeof(*free_slot));
		free_slot->node_id = node_id;
		free_slot->active = true;
		free_slot->state = CEELL_HEALTH_HEALTHY;
	}

	return free_slot;
}

/**
 * Compute the new health state based on how long since last seen.
 */
static enum ceell_health_state compute_state(int64_t now, int64_t last_seen)
{
	int64_t elapsed = now - last_seen;
	int interval = CONFIG_CEELL_DISCOVERY_INTERVAL_MS;

	if (elapsed <= (int64_t)interval) {
		return CEELL_HEALTH_HEALTHY;
	}

	int missed = (int)(elapsed / interval) - 1;

	if (missed <= 2) {
		return CEELL_HEALTH_DEGRADED;
	}

	if (elapsed >= (int64_t)CONFIG_CEELL_PEER_TIMEOUT_MS) {
		return CEELL_HEALTH_LOST;
	}

	return CEELL_HEALTH_CRITICAL;
}

int ceell_health_init(void)
{
	int ret;

	ret = ceell_mutex_init(&health_mutex);
	if (ret < 0) {
		return ret;
	}

	memset(health_table, 0, sizeof(health_table));
	health_cb = NULL;
	health_initialized = true;

	CEELL_LOG_INF("Health monitor initialized (max_peers=%d)",
		      CONFIG_CEELL_DISCOVERY_MAX_PEERS);
	return 0;
}

void ceell_health_update(void)
{
	if (!health_initialized) {
		return;
	}

	struct ceell_peer peers[CONFIG_CEELL_DISCOVERY_MAX_PEERS];
	int peer_count;
	int64_t now = ceell_uptime_get();

	/* Get current peer table from discovery */
	peer_count = ceell_discovery_get_peers(peers, CONFIG_CEELL_DISCOVERY_MAX_PEERS);

	ceell_mutex_lock(&health_mutex);

	/*
	 * Mark all existing health entries as potentially stale.
	 * Peers still in the discovery table will be refreshed below.
	 */
	bool peer_found[CONFIG_CEELL_DISCOVERY_MAX_PEERS];

	memset(peer_found, 0, sizeof(peer_found));

	/* Update health for each discovered peer */
	for (int i = 0; i < peer_count; i++) {
		struct ceell_peer_health *h = find_or_alloc(peers[i].node_id);

		if (!h) {
			continue; /* table full */
		}

		/* Mark which health table slot corresponds to a live peer */
		int idx = (int)(h - health_table);

		peer_found[idx] = true;

		enum ceell_health_state old_state = h->state;
		enum ceell_health_state new_state;

		h->last_seen = peers[i].last_seen;
		new_state = compute_state(now, h->last_seen);

		if (new_state != old_state) {
			h->state = new_state;

			/* Compute missed count for logging */
			if (new_state == CEELL_HEALTH_HEALTHY) {
				h->missed_count = 0;
			} else {
				int64_t elapsed = now - h->last_seen;

				h->missed_count = (int)(elapsed /
					CONFIG_CEELL_DISCOVERY_INTERVAL_MS) - 1;
				if (h->missed_count < 0) {
					h->missed_count = 0;
				}
			}

			CEELL_LOG_WRN("Peer %u health: %d -> %d (missed=%d)",
				      h->node_id, old_state, new_state,
				      h->missed_count);

			if (health_cb) {
				health_cb(h->node_id, old_state, new_state);
			}
		}
	}

	/* Peers that were tracked but are no longer in discovery -> LOST */
	for (int i = 0; i < CONFIG_CEELL_DISCOVERY_MAX_PEERS; i++) {
		if (health_table[i].active && !peer_found[i]) {
			enum ceell_health_state old_state = health_table[i].state;

			if (old_state != CEELL_HEALTH_LOST) {
				health_table[i].state = CEELL_HEALTH_LOST;
				health_table[i].missed_count = -1;

				CEELL_LOG_WRN("Peer %u health: %d -> LOST (expired)",
					      health_table[i].node_id, old_state);

				if (health_cb) {
					health_cb(health_table[i].node_id,
						  old_state, CEELL_HEALTH_LOST);
				}
			}
		}
	}

	ceell_mutex_unlock(&health_mutex);
}

void ceell_health_register_cb(ceell_health_cb_t cb)
{
	ceell_mutex_lock(&health_mutex);
	health_cb = cb;
	ceell_mutex_unlock(&health_mutex);
}

int ceell_health_get_state(uint32_t node_id)
{
	int state = CEELL_HEALTH_LOST;

	ceell_mutex_lock(&health_mutex);

	for (int i = 0; i < CONFIG_CEELL_DISCOVERY_MAX_PEERS; i++) {
		if (health_table[i].active && health_table[i].node_id == node_id) {
			state = health_table[i].state;
			break;
		}
	}

	ceell_mutex_unlock(&health_mutex);

	return state;
}

int ceell_health_get_table(struct ceell_peer_health *out, int max_count)
{
	int count = 0;

	if (!out || max_count <= 0) {
		return 0;
	}

	ceell_mutex_lock(&health_mutex);

	for (int i = 0; i < CONFIG_CEELL_DISCOVERY_MAX_PEERS; i++) {
		if (health_table[i].active && count < max_count) {
			out[count] = health_table[i];
			count++;
		}
	}

	ceell_mutex_unlock(&health_mutex);

	return count;
}
