/*
 * Unit tests for CEELL Safe State Management
 *
 * Tests init, trigger, clear, reason query, and callback behavior.
 */

#include <zephyr/ztest.h>
#include "safe_state.h"

/* ── Callback tracking ────────────────────────────────────────── */

static int cb_call_count;
static bool cb_last_active;
static int cb_last_reason;

static void test_safe_cb(bool active, int reason)
{
	cb_call_count++;
	cb_last_active = active;
	cb_last_reason = reason;
}

/* ── Test fixtures ────────────────────────────────────────────── */

static void safe_state_before(void *fixture)
{
	ARG_UNUSED(fixture);
	ceell_safe_state_init();
	cb_call_count = 0;
	cb_last_active = false;
	cb_last_reason = -1;
	ceell_safe_state_register_cb(test_safe_cb);
}

ZTEST_SUITE(safe_state, NULL, NULL, safe_state_before, NULL, NULL);

/* ── Tests ────────────────────────────────────────────────────── */

ZTEST(safe_state, test_init_not_active)
{
	zassert_false(ceell_safe_state_active(),
		      "should not be in safe state after init");
	zassert_equal(ceell_safe_state_reason(), CEELL_SAFE_NONE,
		      "reason should be NONE after init");
}

ZTEST(safe_state, test_trigger_sets_active)
{
	ceell_safe_state_trigger(CEELL_SAFE_CRITICAL_PEER_LOST);

	zassert_true(ceell_safe_state_active(),
		     "should be active after trigger");
}

ZTEST(safe_state, test_trigger_sets_reason)
{
	ceell_safe_state_trigger(CEELL_SAFE_COMM_FAILURE);

	zassert_equal(ceell_safe_state_reason(), CEELL_SAFE_COMM_FAILURE,
		      "reason should match trigger");
}

ZTEST(safe_state, test_trigger_fires_callback)
{
	ceell_safe_state_trigger(CEELL_SAFE_DEADLINE_BUDGET_EXCEEDED);

	zassert_equal(cb_call_count, 1, "callback should fire once");
	zassert_true(cb_last_active, "callback should report active=true");
	zassert_equal(cb_last_reason, CEELL_SAFE_DEADLINE_BUDGET_EXCEEDED);
}

ZTEST(safe_state, test_clear_resets)
{
	ceell_safe_state_trigger(CEELL_SAFE_CRITICAL_PEER_LOST);
	zassert_true(ceell_safe_state_active());

	ceell_safe_state_clear();

	zassert_false(ceell_safe_state_active(),
		      "should not be active after clear");
	zassert_equal(ceell_safe_state_reason(), CEELL_SAFE_NONE,
		      "reason should be NONE after clear");
}

ZTEST(safe_state, test_clear_fires_callback)
{
	ceell_safe_state_trigger(CEELL_SAFE_COMM_FAILURE);
	cb_call_count = 0;

	ceell_safe_state_clear();

	zassert_equal(cb_call_count, 1, "clear should fire callback");
	zassert_false(cb_last_active, "callback should report active=false");
	zassert_equal(cb_last_reason, CEELL_SAFE_NONE);
}

ZTEST(safe_state, test_double_trigger_no_double_callback)
{
	ceell_safe_state_trigger(CEELL_SAFE_CRITICAL_PEER_LOST);
	zassert_equal(cb_call_count, 1);

	/* Second trigger with same reason should not fire again */
	ceell_safe_state_trigger(CEELL_SAFE_CRITICAL_PEER_LOST);
	zassert_equal(cb_call_count, 1,
		      "duplicate trigger should not fire callback again");
}

ZTEST(safe_state, test_clear_when_not_active_is_noop)
{
	ceell_safe_state_clear();

	zassert_equal(cb_call_count, 0,
		      "clear when not active should not fire callback");
}
