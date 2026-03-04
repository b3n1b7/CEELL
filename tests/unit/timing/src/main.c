/*
 * CEELL Timing Measurement — Unit Tests
 *
 * Tests for timing_init, start/stop, stats calculation (min/max/avg),
 * multiple labels, and edge cases.
 *
 * Runs on native_sim where CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=1000000,
 * so 1 cycle = 1 microsecond.
 */

#include <zephyr/ztest.h>
#include "timing.h"
#include "osal_time.h"

ZTEST_SUITE(timing, NULL, NULL, NULL, NULL, NULL);

/* ── Initialization ─────────────────────────────────────────────── */

ZTEST(timing, test_init)
{
	/* Should not crash; double-init should be safe */
	ceell_timing_init();
	ceell_timing_init();
}

/* ── Start / Stop basic ─────────────────────────────────────────── */

ZTEST(timing, test_start_stop_basic)
{
	struct ceell_timing_stats stats;
	uint32_t handle;

	ceell_timing_init();

	ceell_timing_start("basic_op", &handle);
	/* Small busy-wait so cycle counter advances */
	volatile int dummy = 0;
	for (int i = 0; i < 100; i++) {
		dummy += i;
	}
	ceell_timing_stop("basic_op", handle);

	int ret = ceell_timing_get_stats("basic_op", &stats);

	zassert_equal(ret, 0, "Expected stats for 'basic_op'");
	zassert_equal(stats.count, 1, "Expected exactly 1 sample");
	/* On native_sim, duration should be >= 0 */
	zassert_true(stats.min_us == stats.max_us,
		     "Single sample: min should equal max");
	zassert_true(stats.avg_us == stats.min_us,
		     "Single sample: avg should equal min");
}

/* ── Multiple samples, same label ───────────────────────────────── */

ZTEST(timing, test_multiple_samples_same_label)
{
	struct ceell_timing_stats stats;
	uint32_t handle;

	ceell_timing_init();

	for (int i = 0; i < 5; i++) {
		ceell_timing_start("multi", &handle);
		volatile int dummy = 0;
		for (int j = 0; j < (i + 1) * 50; j++) {
			dummy += j;
		}
		ceell_timing_stop("multi", handle);
	}

	int ret = ceell_timing_get_stats("multi", &stats);

	zassert_equal(ret, 0, "Expected stats for 'multi'");
	zassert_equal(stats.count, 5, "Expected 5 samples");
	zassert_true(stats.min_us <= stats.avg_us,
		     "min should be <= avg");
	zassert_true(stats.avg_us <= stats.max_us,
		     "avg should be <= max");
}

/* ── Multiple labels ────────────────────────────────────────────── */

ZTEST(timing, test_multiple_labels)
{
	struct ceell_timing_stats stats_a;
	struct ceell_timing_stats stats_b;
	uint32_t handle;

	ceell_timing_init();

	ceell_timing_start("label_a", &handle);
	ceell_timing_stop("label_a", handle);

	ceell_timing_start("label_b", &handle);
	ceell_timing_stop("label_b", handle);

	ceell_timing_start("label_a", &handle);
	ceell_timing_stop("label_a", handle);

	zassert_equal(ceell_timing_get_stats("label_a", &stats_a), 0);
	zassert_equal(ceell_timing_get_stats("label_b", &stats_b), 0);

	zassert_equal(stats_a.count, 2, "label_a should have 2 samples");
	zassert_equal(stats_b.count, 1, "label_b should have 1 sample");
}

/* ── Stats for unknown label ────────────────────────────────────── */

ZTEST(timing, test_stats_unknown_label)
{
	struct ceell_timing_stats stats;

	ceell_timing_init();

	int ret = ceell_timing_get_stats("nonexistent", &stats);

	zassert_equal(ret, -1, "Unknown label should return -1");
}

/* ── Null arguments ─────────────────────────────────────────────── */

ZTEST(timing, test_null_handle)
{
	ceell_timing_init();

	/* Should not crash */
	ceell_timing_start("null_test", NULL);
}

ZTEST(timing, test_null_label_stop)
{
	uint32_t handle;

	ceell_timing_init();

	ceell_timing_start("dummy", &handle);
	/* Stop with NULL label should be a no-op */
	ceell_timing_stop(NULL, handle);

	struct ceell_timing_stats stats;
	int ret = ceell_timing_get_stats("dummy", &stats);

	/* No sample should have been stored for "dummy" via stop */
	zassert_equal(ret, -1, "NULL label stop should not store sample");
}

ZTEST(timing, test_null_stats_args)
{
	ceell_timing_init();

	zassert_equal(ceell_timing_get_stats(NULL, NULL), -1,
		      "NULL args should return -1");

	struct ceell_timing_stats stats;

	zassert_equal(ceell_timing_get_stats(NULL, &stats), -1,
		      "NULL label should return -1");
	zassert_equal(ceell_timing_get_stats("test", NULL), -1,
		      "NULL stats should return -1");
}

/* ── Print stats (smoke test — should not crash) ────────────────── */

ZTEST(timing, test_print_stats)
{
	uint32_t handle;

	ceell_timing_init();

	ceell_timing_start("print_test", &handle);
	ceell_timing_stop("print_test", handle);

	/* Should not crash */
	ceell_timing_print_stats();
}

/* ── Ring buffer wrap-around ────────────────────────────────────── */

ZTEST(timing, test_ring_buffer_overflow)
{
	struct ceell_timing_stats stats;
	uint32_t handle;

	ceell_timing_init();

	/* Fill the ring buffer beyond capacity (default 64 slots) */
	for (int i = 0; i < 80; i++) {
		ceell_timing_start("overflow", &handle);
		ceell_timing_stop("overflow", handle);
	}

	int ret = ceell_timing_get_stats("overflow", &stats);

	zassert_equal(ret, 0, "Should have stats after overflow");
	/* After 80 inserts into a 64-slot buffer, we should have
	 * at most 64 samples retained. */
	zassert_true(stats.count <= 64, "Count should be capped at ring size");
	zassert_true(stats.count > 0, "Should have at least some samples");
}
