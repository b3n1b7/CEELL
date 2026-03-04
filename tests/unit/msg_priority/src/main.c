#include <zephyr/ztest.h>
#include <stdbool.h>
#include "msg_priority.h"

ZTEST_SUITE(msg_priority, NULL, NULL, NULL, NULL, NULL);

/* ── Enum values ──────────────────────────────────────────────── */

ZTEST(msg_priority, test_critical_is_zero)
{
	zassert_equal(CEELL_MSG_CRITICAL, 0,
		      "CRITICAL must be 0 (highest priority)");
}

ZTEST(msg_priority, test_normal_is_one)
{
	zassert_equal(CEELL_MSG_NORMAL, 1,
		      "NORMAL must be 1");
}

ZTEST(msg_priority, test_bulk_is_two)
{
	zassert_equal(CEELL_MSG_BULK, 2,
		      "BULK must be 2 (lowest priority)");
}

ZTEST(msg_priority, test_priority_count)
{
	zassert_equal(CEELL_MSG_PRIORITY_COUNT, 3,
		      "There must be exactly 3 priority levels");
}

/* ── Ordering: CRITICAL < NORMAL < BULK ──────────────────────── */

ZTEST(msg_priority, test_critical_higher_than_normal)
{
	zassert_true(CEELL_MSG_CRITICAL < CEELL_MSG_NORMAL,
		     "CRITICAL must have lower numeric value than NORMAL");
}

ZTEST(msg_priority, test_normal_higher_than_bulk)
{
	zassert_true(CEELL_MSG_NORMAL < CEELL_MSG_BULK,
		     "NORMAL must have lower numeric value than BULK");
}

ZTEST(msg_priority, test_critical_highest)
{
	zassert_true(CEELL_MSG_CRITICAL < CEELL_MSG_BULK,
		     "CRITICAL must have lowest numeric value (highest prio)");
}

/* ── Validation ──────────────────────────────────────────────── */

ZTEST(msg_priority, test_valid_critical)
{
	zassert_true(ceell_msg_priority_valid(CEELL_MSG_CRITICAL));
}

ZTEST(msg_priority, test_valid_normal)
{
	zassert_true(ceell_msg_priority_valid(CEELL_MSG_NORMAL));
}

ZTEST(msg_priority, test_valid_bulk)
{
	zassert_true(ceell_msg_priority_valid(CEELL_MSG_BULK));
}

ZTEST(msg_priority, test_invalid_negative)
{
	zassert_false(ceell_msg_priority_valid(-1));
}

ZTEST(msg_priority, test_invalid_too_high)
{
	zassert_false(ceell_msg_priority_valid(CEELL_MSG_PRIORITY_COUNT));
}

ZTEST(msg_priority, test_invalid_large)
{
	zassert_false(ceell_msg_priority_valid(99));
}

/* ── Name strings ────────────────────────────────────────────── */

ZTEST(msg_priority, test_name_critical)
{
	zassert_str_equal(ceell_msg_priority_name(CEELL_MSG_CRITICAL),
			  "CRITICAL");
}

ZTEST(msg_priority, test_name_normal)
{
	zassert_str_equal(ceell_msg_priority_name(CEELL_MSG_NORMAL),
			  "NORMAL");
}

ZTEST(msg_priority, test_name_bulk)
{
	zassert_str_equal(ceell_msg_priority_name(CEELL_MSG_BULK),
			  "BULK");
}

ZTEST(msg_priority, test_name_unknown)
{
	zassert_str_equal(ceell_msg_priority_name(99),
			  "UNKNOWN");
}
