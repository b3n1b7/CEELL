/*
 * Unit tests for CEELL Health Monitor
 *
 * Stubs out the discovery module and OSAL primitives to test
 * health state transitions and callback invocations in isolation.
 */

#include <zephyr/ztest.h>
#include <string.h>
#include "health_monitor.h"
#include "discovery.h"

/* ── Mock discovery state ─────────────────────────────────────── */

static struct ceell_peer mock_peers[8];
static int mock_peer_count;

int ceell_discovery_get_peers(struct ceell_peer *out, int max_peers)
{
	int count = (mock_peer_count < max_peers) ? mock_peer_count : max_peers;

	memcpy(out, mock_peers, count * sizeof(struct ceell_peer));
	return count;
}

int ceell_discovery_peer_count(void)
{
	return mock_peer_count;
}

int ceell_discovery_expire_peers(void)
{
	return 0;
}

/* ── Callback tracking ────────────────────────────────────────── */

static int cb_call_count;
static uint32_t cb_last_node_id;
static int cb_last_old_state;
static int cb_last_new_state;

static void test_health_cb(uint32_t node_id, int old_state, int new_state)
{
	cb_call_count++;
	cb_last_node_id = node_id;
	cb_last_old_state = old_state;
	cb_last_new_state = new_state;
}

/* ── Test fixtures ────────────────────────────────────────────── */

static void reset_mocks(void)
{
	memset(mock_peers, 0, sizeof(mock_peers));
	mock_peer_count = 0;
	cb_call_count = 0;
	cb_last_node_id = 0;
	cb_last_old_state = -1;
	cb_last_new_state = -1;
}

static void *health_suite_setup(void)
{
	return NULL;
}

static void health_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_mocks();
	ceell_health_init();
	ceell_health_register_cb(test_health_cb);
}

ZTEST_SUITE(health_monitor, NULL, health_suite_setup, health_before, NULL, NULL);

/* ── Tests ────────────────────────────────────────────────────── */

ZTEST(health_monitor, test_init_succeeds)
{
	int ret = ceell_health_init();
	zassert_equal(ret, 0, "health init should succeed");
}

ZTEST(health_monitor, test_no_peers_no_crash)
{
	/* Update with empty peer table should not crash */
	mock_peer_count = 0;
	ceell_health_update();
	zassert_equal(cb_call_count, 0, "no callbacks for empty peer table");
}

ZTEST(health_monitor, test_new_peer_healthy)
{
	/* Add a peer that was just seen */
	mock_peers[0].node_id = 10;
	mock_peers[0].last_seen = k_uptime_get();
	mock_peer_count = 1;

	ceell_health_update();

	/* New peer starts HEALTHY — no transition callback expected
	 * because initial state is already HEALTHY */
	int state = ceell_health_get_state(10);
	zassert_equal(state, CEELL_HEALTH_HEALTHY, "new peer should be HEALTHY");
}

ZTEST(health_monitor, test_transition_to_degraded)
{
	/* Add a peer seen 2 intervals ago */
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 20;
	mock_peers[0].last_seen = now;
	mock_peer_count = 1;

	/* First update: establish as HEALTHY */
	ceell_health_update();
	zassert_equal(ceell_health_get_state(20), CEELL_HEALTH_HEALTHY);

	/* Simulate time passing: peer seen 2 intervals ago */
	mock_peers[0].last_seen = now - (CONFIG_CEELL_DISCOVERY_INTERVAL_MS * 2);

	ceell_health_update();

	zassert_equal(ceell_health_get_state(20), CEELL_HEALTH_DEGRADED,
		      "peer should be DEGRADED after 2 missed intervals");
	zassert_equal(cb_call_count, 1, "callback should fire on transition");
	zassert_equal(cb_last_node_id, 20);
	zassert_equal(cb_last_old_state, CEELL_HEALTH_HEALTHY);
	zassert_equal(cb_last_new_state, CEELL_HEALTH_DEGRADED);
}

ZTEST(health_monitor, test_transition_to_critical)
{
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 30;
	mock_peers[0].last_seen = now;
	mock_peer_count = 1;

	/* Establish as HEALTHY */
	ceell_health_update();

	/* 4 intervals ago -> CRITICAL (3+ missed, but not at timeout) */
	mock_peers[0].last_seen = now - (CONFIG_CEELL_DISCOVERY_INTERVAL_MS * 4);

	ceell_health_update();

	zassert_equal(ceell_health_get_state(30), CEELL_HEALTH_CRITICAL,
		      "peer should be CRITICAL after 4 missed intervals");
}

ZTEST(health_monitor, test_transition_to_lost)
{
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 40;
	mock_peers[0].last_seen = now;
	mock_peer_count = 1;

	ceell_health_update();

	/* Peer exceeds timeout -> LOST */
	mock_peers[0].last_seen = now - CONFIG_CEELL_PEER_TIMEOUT_MS - 1;

	ceell_health_update();

	zassert_equal(ceell_health_get_state(40), CEELL_HEALTH_LOST,
		      "peer should be LOST after timeout");
}

ZTEST(health_monitor, test_peer_removed_becomes_lost)
{
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 50;
	mock_peers[0].last_seen = now;
	mock_peer_count = 1;

	ceell_health_update();
	zassert_equal(ceell_health_get_state(50), CEELL_HEALTH_HEALTHY);

	/* Remove peer from discovery table */
	mock_peer_count = 0;

	ceell_health_update();

	zassert_equal(ceell_health_get_state(50), CEELL_HEALTH_LOST,
		      "peer removed from table should transition to LOST");
	zassert_true(cb_call_count >= 1, "callback should fire on LOST");
}

ZTEST(health_monitor, test_recovery_to_healthy)
{
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 60;
	mock_peers[0].last_seen = now - (CONFIG_CEELL_DISCOVERY_INTERVAL_MS * 3);
	mock_peer_count = 1;

	/* Start in DEGRADED state */
	ceell_health_update();
	zassert_equal(ceell_health_get_state(60), CEELL_HEALTH_DEGRADED);

	/* Peer comes back */
	mock_peers[0].last_seen = k_uptime_get();
	cb_call_count = 0;

	ceell_health_update();

	zassert_equal(ceell_health_get_state(60), CEELL_HEALTH_HEALTHY,
		      "peer should recover to HEALTHY when seen again");
	zassert_equal(cb_call_count, 1, "callback should fire on recovery");
}

ZTEST(health_monitor, test_unknown_peer_returns_lost)
{
	int state = ceell_health_get_state(999);
	zassert_equal(state, CEELL_HEALTH_LOST,
		      "unknown peer should report LOST");
}

ZTEST(health_monitor, test_callback_null_safe)
{
	ceell_health_register_cb(NULL);

	int64_t now = k_uptime_get();
	mock_peers[0].node_id = 70;
	mock_peers[0].last_seen = now - (CONFIG_CEELL_DISCOVERY_INTERVAL_MS * 4);
	mock_peer_count = 1;

	/* Should not crash with NULL callback */
	ceell_health_update();

	zassert_equal(ceell_health_get_state(70), CEELL_HEALTH_CRITICAL);
}

ZTEST(health_monitor, test_get_table)
{
	int64_t now = k_uptime_get();

	mock_peers[0].node_id = 80;
	mock_peers[0].last_seen = now;
	mock_peers[1].node_id = 81;
	mock_peers[1].last_seen = now;
	mock_peer_count = 2;

	ceell_health_update();

	struct ceell_peer_health table[8];
	int count = ceell_health_get_table(table, 8);

	zassert_equal(count, 2, "should have 2 health entries");
	zassert_equal(table[0].node_id, 80);
	zassert_equal(table[1].node_id, 81);
}
