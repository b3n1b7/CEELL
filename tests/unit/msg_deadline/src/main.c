#include <zephyr/ztest.h>
#include "messaging.h"
#include "msg_deadline.h"
#include "osal_time.h"

ZTEST_SUITE(msg_deadline, NULL, NULL, NULL, NULL, NULL);

/* ── No deadline (deadline_ms == 0) ──────────────────────────── */

ZTEST(msg_deadline, test_no_deadline_remaining_is_zero)
{
	struct ceell_msg msg = {
		.deadline_ms = 0,
		.send_time_ms = 0,
	};

	zassert_equal(ceell_msg_deadline_remaining(&msg), 0,
		      "No deadline should return 0 remaining");
}

ZTEST(msg_deadline, test_no_deadline_not_expired)
{
	struct ceell_msg msg = {
		.deadline_ms = 0,
		.send_time_ms = 0,
	};

	zassert_false(ceell_msg_deadline_expired(&msg),
		      "No deadline should never be expired");
}

/* ── NULL message ────────────────────────────────────────────── */

ZTEST(msg_deadline, test_null_msg_remaining)
{
	zassert_equal(ceell_msg_deadline_remaining(NULL), 0,
		      "NULL msg should return 0 remaining");
}

ZTEST(msg_deadline, test_null_msg_not_expired)
{
	zassert_false(ceell_msg_deadline_expired(NULL),
		      "NULL msg should not be expired");
}

/* ── Future deadline (should not be expired) ─────────────────── */

ZTEST(msg_deadline, test_future_deadline_not_expired)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 10000,       /* 10 seconds from send */
		.send_time_ms = now,        /* sent right now */
	};

	zassert_false(ceell_msg_deadline_expired(&msg),
		      "Deadline 10s in the future should not be expired");
}

ZTEST(msg_deadline, test_future_deadline_remaining_positive)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 10000,
		.send_time_ms = now,
	};

	int remaining = ceell_msg_deadline_remaining(&msg);

	zassert_true(remaining > 0,
		     "Remaining ms should be positive for future deadline "
		     "(got %d)", remaining);
	/* Should be close to 10000, but allow some slack for test execution */
	zassert_true(remaining <= 10000,
		     "Remaining should be <= 10000 (got %d)", remaining);
}

/* ── Past deadline (should be expired) ───────────────────────── */

ZTEST(msg_deadline, test_past_deadline_expired)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 5,              /* 5 ms deadline */
		.send_time_ms = now - 1000,    /* sent 1 second ago */
	};

	zassert_true(ceell_msg_deadline_expired(&msg),
		     "Deadline 1s in the past should be expired");
}

ZTEST(msg_deadline, test_past_deadline_remaining_negative)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 5,
		.send_time_ms = now - 1000,
	};

	int remaining = ceell_msg_deadline_remaining(&msg);

	zassert_true(remaining < 0,
		     "Remaining should be negative for expired deadline "
		     "(got %d)", remaining);
}

/* ── Edge: deadline_ms == 1 with very recent send ────────────── */

ZTEST(msg_deadline, test_tiny_deadline_recent_send)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 60000,  /* 60 seconds — well in the future */
		.send_time_ms = now,
	};

	/* Should not be expired right after sending */
	zassert_false(ceell_msg_deadline_expired(&msg));

	int remaining = ceell_msg_deadline_remaining(&msg);

	zassert_true(remaining > 0,
		     "Should have positive remaining time");
}

/* ── Large deadline value ────────────────────────────────────── */

ZTEST(msg_deadline, test_max_deadline)
{
	int64_t now = ceell_uptime_get();

	struct ceell_msg msg = {
		.deadline_ms = 65535,   /* max uint16_t */
		.send_time_ms = now,
	};

	zassert_false(ceell_msg_deadline_expired(&msg),
		      "Max deadline should not be immediately expired");

	int remaining = ceell_msg_deadline_remaining(&msg);

	zassert_true(remaining > 60000,
		     "Max deadline should have >60s remaining (got %d)",
		     remaining);
}

/* ── Consistency: expired iff remaining < 0 ──────────────────── */

ZTEST(msg_deadline, test_expired_consistent_with_remaining)
{
	int64_t now = ceell_uptime_get();

	/* Not expired */
	struct ceell_msg future = {
		.deadline_ms = 30000,
		.send_time_ms = now,
	};

	zassert_false(ceell_msg_deadline_expired(&future));
	zassert_true(ceell_msg_deadline_remaining(&future) > 0);

	/* Expired */
	struct ceell_msg past = {
		.deadline_ms = 1,
		.send_time_ms = now - 5000,
	};

	zassert_true(ceell_msg_deadline_expired(&past));
	zassert_true(ceell_msg_deadline_remaining(&past) < 0);
}
