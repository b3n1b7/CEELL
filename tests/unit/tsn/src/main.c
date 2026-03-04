/*
 * CEELL TSN Traffic Class Mapping — Unit Tests
 *
 * Tests for PCP mapping, tsn_available, socket priority, and
 * edge cases.
 */

#include <zephyr/ztest.h>
#include "tsn.h"
#include "msg_priority.h"

ZTEST_SUITE(tsn, NULL, NULL, NULL, NULL, NULL);

/* ── TSN init ───────────────────────────────────────────────────── */

ZTEST(tsn, test_init)
{
	int ret = ceell_tsn_init();

	zassert_equal(ret, 0, "TSN init should succeed");
}

/* ── PCP mapping: CRITICAL -> 7 ─────────────────────────────────── */

ZTEST(tsn, test_pcp_critical)
{
	int pcp = ceell_tsn_get_pcp(CEELL_MSG_CRITICAL);

	zassert_equal(pcp, 7, "CRITICAL should map to PCP 7 (Network Control)");
}

/* ── PCP mapping: NORMAL -> 4 ───────────────────────────────────── */

ZTEST(tsn, test_pcp_normal)
{
	int pcp = ceell_tsn_get_pcp(CEELL_MSG_NORMAL);

	zassert_equal(pcp, 4, "NORMAL should map to PCP 4 (Controlled Load)");
}

/* ── PCP mapping: BULK -> 1 ─────────────────────────────────────── */

ZTEST(tsn, test_pcp_bulk)
{
	int pcp = ceell_tsn_get_pcp(CEELL_MSG_BULK);

	zassert_equal(pcp, 1, "BULK should map to PCP 1 (Background)");
}

/* ── PCP mapping: invalid priority ──────────────────────────────── */

ZTEST(tsn, test_pcp_invalid)
{
	zassert_equal(ceell_tsn_get_pcp(-1), -1,
		      "Negative priority should return -1");
	zassert_equal(ceell_tsn_get_pcp(CEELL_MSG_PRIORITY_COUNT), -1,
		      "Out-of-range priority should return -1");
	zassert_equal(ceell_tsn_get_pcp(99), -1,
		      "Way out-of-range priority should return -1");
}

/* ── TSN available (should be false — no hardware) ──────────────── */

ZTEST(tsn, test_available_false)
{
	zassert_false(ceell_tsn_available(),
		      "TSN should not be available (no hardware)");
}

/* ── Set socket priority (no-op when TSN unavailable) ───────────── */

ZTEST(tsn, test_set_socket_priority_noop)
{
	/* Should succeed even without real socket — it's a no-op */
	int ret = ceell_tsn_set_socket_priority(42, CEELL_MSG_CRITICAL);

	zassert_equal(ret, 0,
		      "set_socket_priority should return 0 (no-op)");
}

/* ── Set socket priority with invalid priority ──────────────────── */

ZTEST(tsn, test_set_socket_priority_invalid)
{
	int ret = ceell_tsn_set_socket_priority(42, -1);

	zassert_equal(ret, -1,
		      "Invalid priority should return -1");
}
