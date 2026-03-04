/*
 * CEELL Timing Budget — Unit Tests
 *
 * Tests for budget define, check within budget, check violation,
 * violation counting, report output, and edge cases.
 */

#include <zephyr/ztest.h>
#include "timing_budget.h"

ZTEST_SUITE(timing_budget, NULL, NULL, NULL, NULL, NULL);

/* ── Define a budget ────────────────────────────────────────────── */

ZTEST(timing_budget, test_define_basic)
{
	int ret = ceell_timing_budget_define("test_op", 1000);

	zassert_equal(ret, 0, "Should succeed defining a budget");
}

/* ── Check within budget ────────────────────────────────────────── */

ZTEST(timing_budget, test_check_within_budget)
{
	ceell_timing_budget_define("within", 500);

	int ret = ceell_timing_budget_check("within", 250);

	zassert_equal(ret, 0, "250 us should be within 500 us budget");
}

/* ── Check exactly at budget ────────────────────────────────────── */

ZTEST(timing_budget, test_check_at_budget)
{
	ceell_timing_budget_define("exact", 100);

	int ret = ceell_timing_budget_check("exact", 100);

	zassert_equal(ret, 0, "Exactly at budget should pass (not violation)");
}

/* ── Check violation ────────────────────────────────────────────── */

ZTEST(timing_budget, test_check_violation)
{
	ceell_timing_budget_define("over", 100);

	int ret = ceell_timing_budget_check("over", 150);

	zassert_equal(ret, -1, "150 us should violate 100 us budget");
}

/* ── Violation count increments ─────────────────────────────────── */

ZTEST(timing_budget, test_violation_count)
{
	ceell_timing_budget_define("count_test", 50);

	ceell_timing_budget_check("count_test", 60);  /* violation 1 */
	ceell_timing_budget_check("count_test", 70);  /* violation 2 */
	ceell_timing_budget_check("count_test", 80);  /* violation 3 */

	/* Verify by checking that a 4th violation still returns -1 */
	int ret = ceell_timing_budget_check("count_test", 90);

	zassert_equal(ret, -1, "Should still report violation");
}

/* ── Check unknown label ────────────────────────────────────────── */

ZTEST(timing_budget, test_check_unknown_label)
{
	int ret = ceell_timing_budget_check("unknown_label", 50);

	zassert_equal(ret, -1, "Unknown label should return -1");
}

/* ── Null arguments ─────────────────────────────────────────────── */

ZTEST(timing_budget, test_define_null_label)
{
	int ret = ceell_timing_budget_define(NULL, 100);

	zassert_equal(ret, -1, "NULL label should fail");
}

ZTEST(timing_budget, test_define_zero_budget)
{
	int ret = ceell_timing_budget_define("zero", 0);

	zassert_equal(ret, -1, "Zero budget should fail");
}

/* ── Report (smoke test — should not crash) ─────────────────────── */

ZTEST(timing_budget, test_report)
{
	ceell_timing_budget_define("report_a", 200);
	ceell_timing_budget_define("report_b", 500);

	ceell_timing_budget_check("report_a", 300);  /* violation */
	ceell_timing_budget_check("report_b", 100);  /* ok */

	/* Should not crash */
	ceell_timing_budget_report();
}
