/*
 * CEELL Health Monitor
 *
 * Tracks the health state of all discovered peers based on
 * discovery heartbeat timing. Provides callback notifications
 * on state transitions and thread-safe state queries.
 */

#ifndef CEELL_HEALTH_MONITOR_H
#define CEELL_HEALTH_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/** Peer health states */
enum ceell_health_state {
	CEELL_HEALTH_HEALTHY  = 0,  /**< Peer is responding normally */
	CEELL_HEALTH_DEGRADED = 1,  /**< 1-2 missed heartbeat intervals */
	CEELL_HEALTH_CRITICAL = 2,  /**< 3+ missed heartbeat intervals */
	CEELL_HEALTH_LOST     = 3,  /**< Exceeds peer timeout — considered gone */
};

/** Per-peer health tracking information */
struct ceell_peer_health {
	uint32_t node_id;            /**< Peer node ID */
	enum ceell_health_state state; /**< Current health state */
	int64_t last_seen;           /**< Uptime ms when last seen in peer table */
	int missed_count;            /**< Number of missed heartbeat intervals */
	bool active;                 /**< Slot in use */
};

/**
 * Callback type for health state transitions.
 *
 * @param node_id   Peer whose health state changed
 * @param old_state Previous health state
 * @param new_state New health state
 */
typedef void (*ceell_health_cb_t)(uint32_t node_id, int old_state, int new_state);

/**
 * Initialize the health monitor.
 *
 * @return 0 on success, negative errno on failure
 */
int ceell_health_init(void);

/**
 * Update health states for all tracked peers.
 * Should be called once per lifecycle tick.
 * Reads the discovery peer table and computes health transitions.
 */
void ceell_health_update(void);

/**
 * Register a callback for health state transitions.
 * Only one callback is supported; subsequent calls overwrite.
 *
 * @param cb  Callback function (NULL to unregister)
 */
void ceell_health_register_cb(ceell_health_cb_t cb);

/**
 * Get the current health state of a peer.
 *
 * @param node_id  Peer node ID to query
 * @return Health state, or CEELL_HEALTH_LOST if peer is unknown
 */
int ceell_health_get_state(uint32_t node_id);

/**
 * Get a copy of the health table for inspection/testing.
 *
 * @param out       Array to fill
 * @param max_count Size of the output array
 * @return Number of entries copied
 */
int ceell_health_get_table(struct ceell_peer_health *out, int max_count);

#endif /* CEELL_HEALTH_MONITOR_H */
