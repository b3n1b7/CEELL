/*
 * Unit tests for CEELL Dual-Path Redundant Messaging
 *
 * Tests the deduplication ring buffer in isolation. The ring buffer
 * logic does not depend on networking, so these tests verify the core
 * dedup algorithm including new-seq acceptance, duplicate detection,
 * and ring wrapping behavior.
 *
 * ceell_msg_redundancy_init() is called in setup. The socket creation
 * will fail on native_sim without a network stack, but the mutex and
 * ring buffer are initialized before the socket attempt, so the dedup
 * functions work correctly regardless.
 */

#include <zephyr/ztest.h>
#include "msg_redundancy.h"

static void *dedup_suite_setup(void)
{
	/*
	 * Init the redundancy subsystem. This initializes the mutex and
	 * clears the ring buffer. The socket creation may fail on native_sim
	 * (no net stack), but that is expected — we only test dedup logic.
	 */
	(void)ceell_msg_redundancy_init();
	return NULL;
}

ZTEST_SUITE(msg_redundancy, NULL, dedup_suite_setup, NULL, NULL, NULL);

ZTEST(msg_redundancy, test_new_seq_passes)
{
	/* First time seeing seq 100 -- should return false (not duplicate) */
	bool dup = ceell_msg_dedup_check(100);
	zassert_false(dup, "first occurrence should not be a duplicate");
}

ZTEST(msg_redundancy, test_duplicate_seq_caught)
{
	/* Insert seq 200, then check again */
	ceell_msg_dedup_check(200);

	bool dup = ceell_msg_dedup_check(200);
	zassert_true(dup, "second occurrence should be caught as duplicate");
}

ZTEST(msg_redundancy, test_different_seqs_pass)
{
	bool dup1 = ceell_msg_dedup_check(300);
	bool dup2 = ceell_msg_dedup_check(301);
	bool dup3 = ceell_msg_dedup_check(302);

	zassert_false(dup1, "seq 300 should pass");
	zassert_false(dup2, "seq 301 should pass");
	zassert_false(dup3, "seq 302 should pass");
}

ZTEST(msg_redundancy, test_ring_wraps)
{
	/*
	 * Fill the ring buffer beyond capacity.
	 * After 32 entries, the oldest should be evicted.
	 * Seq 1000 will be evicted after 32 more inserts.
	 */
	uint32_t base = 1000;

	ceell_msg_dedup_check(base);  /* insert seq 1000 */

	/* Fill remaining slots + overflow */
	for (uint32_t i = 1; i <= CEELL_MSG_DEDUP_RING_SIZE; i++) {
		ceell_msg_dedup_check(base + i);
	}

	/* seq 1000 should have been evicted -- checking it again should
	 * return false (not found = not duplicate) */
	bool dup = ceell_msg_dedup_check(base);
	zassert_false(dup,
		      "evicted seq should not be found as duplicate");
}

ZTEST(msg_redundancy, test_recent_still_detected)
{
	/*
	 * Insert a batch of sequences, then verify that the most recent
	 * ones are still detected as duplicates.
	 */
	uint32_t base = 2000;

	for (uint32_t i = 0; i < CEELL_MSG_DEDUP_RING_SIZE; i++) {
		ceell_msg_dedup_check(base + i);
	}

	/* The last inserted should still be in the ring */
	bool dup = ceell_msg_dedup_check(base + CEELL_MSG_DEDUP_RING_SIZE - 1);
	zassert_true(dup, "most recent seq should still be in ring");
}

ZTEST(msg_redundancy, test_zero_seq_handled)
{
	/* Sequence number 0 should work correctly */
	bool dup1 = ceell_msg_dedup_check(0);
	bool dup2 = ceell_msg_dedup_check(0);

	zassert_false(dup1, "first seq 0 should pass");
	zassert_true(dup2, "second seq 0 should be duplicate");
}
