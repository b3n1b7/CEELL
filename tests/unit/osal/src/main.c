/*
 * CEELL OSAL Unit Tests
 *
 * Tests each OSAL primitive on native_sim to verify the abstraction
 * layer correctly wraps the underlying RTOS.
 *
 * REQ-OSAL-002, REQ-OSAL-003, REQ-OSAL-004
 * REQ-SAF-005
 */

#include <zephyr/ztest.h>
#include "osal.h"

/* ── Mutex Tests ─────────────────────────────────────────────── */

ZTEST_SUITE(osal_mutex, NULL, NULL, NULL, NULL, NULL);

ZTEST(osal_mutex, test_mutex_init)
{
	ceell_mutex_t mutex;
	int ret = ceell_mutex_init(&mutex);

	zassert_equal(ret, 0, "mutex init should return 0");
}

ZTEST(osal_mutex, test_mutex_lock_unlock)
{
	ceell_mutex_t mutex;

	ceell_mutex_init(&mutex);

	int ret = ceell_mutex_lock(&mutex);

	zassert_equal(ret, 0, "mutex lock should return 0");

	ret = ceell_mutex_unlock(&mutex);
	zassert_equal(ret, 0, "mutex unlock should return 0");
}

ZTEST(osal_mutex, test_mutex_double_lock_same_thread)
{
	ceell_mutex_t mutex;

	ceell_mutex_init(&mutex);
	ceell_mutex_lock(&mutex);

	/* Zephyr mutexes are reentrant — same thread can lock again */
	int ret = ceell_mutex_lock(&mutex);

	zassert_equal(ret, 0, "reentrant lock should succeed");

	ceell_mutex_unlock(&mutex);
	ceell_mutex_unlock(&mutex);
}

/* ── Semaphore Tests ─────────────────────────────────────────── */

ZTEST_SUITE(osal_sem, NULL, NULL, NULL, NULL, NULL);

ZTEST(osal_sem, test_sem_init)
{
	ceell_sem_t sem;
	int ret = ceell_sem_init(&sem, 0, 1);

	zassert_equal(ret, 0, "sem init should return 0");
}

ZTEST(osal_sem, test_sem_give_take)
{
	ceell_sem_t sem;

	ceell_sem_init(&sem, 0, 1);
	ceell_sem_give(&sem);

	int ret = ceell_sem_take(&sem, CEELL_NO_WAIT);

	zassert_equal(ret, 0, "sem take should succeed after give");
}

ZTEST(osal_sem, test_sem_take_timeout)
{
	ceell_sem_t sem;

	ceell_sem_init(&sem, 0, 1);

	/* Take without give — should timeout */
	int ret = ceell_sem_take(&sem, CEELL_MSEC(10));

	zassert_not_equal(ret, 0, "sem take should timeout when empty");
}

ZTEST(osal_sem, test_sem_reset)
{
	ceell_sem_t sem;

	ceell_sem_init(&sem, 0, 5);
	ceell_sem_give(&sem);
	ceell_sem_give(&sem);
	ceell_sem_reset(&sem);

	/* After reset, take should fail */
	int ret = ceell_sem_take(&sem, CEELL_NO_WAIT);

	zassert_not_equal(ret, 0, "sem take should fail after reset");
}

ZTEST(osal_sem, test_sem_initial_count)
{
	ceell_sem_t sem;

	ceell_sem_init(&sem, 3, 5);

	/* Should be able to take 3 times */
	zassert_equal(ceell_sem_take(&sem, CEELL_NO_WAIT), 0, "take 1");
	zassert_equal(ceell_sem_take(&sem, CEELL_NO_WAIT), 0, "take 2");
	zassert_equal(ceell_sem_take(&sem, CEELL_NO_WAIT), 0, "take 3");

	/* 4th take should fail */
	zassert_not_equal(ceell_sem_take(&sem, CEELL_NO_WAIT), 0, "take 4");
}

/* ── Message Queue Tests ─────────────────────────────────────── */

ZTEST_SUITE(osal_msgq, NULL, NULL, NULL, NULL, NULL);

struct test_msg {
	uint32_t id;
	uint32_t data;
};

ZTEST(osal_msgq, test_msgq_init_put_get)
{
	ceell_msgq_t msgq;
	static char buf[sizeof(struct test_msg) * 4];

	int ret = ceell_msgq_init(&msgq, buf, sizeof(struct test_msg), 4);

	zassert_equal(ret, 0, "msgq init should return 0");

	struct test_msg send = { .id = 42, .data = 100 };

	ret = ceell_msgq_put(&msgq, &send, CEELL_NO_WAIT);
	zassert_equal(ret, 0, "msgq put should succeed");

	struct test_msg recv = { 0 };

	ret = ceell_msgq_get(&msgq, &recv, CEELL_NO_WAIT);
	zassert_equal(ret, 0, "msgq get should succeed");
	zassert_equal(recv.id, 42, "id should match");
	zassert_equal(recv.data, 100, "data should match");
}

ZTEST(osal_msgq, test_msgq_empty_get_timeout)
{
	ceell_msgq_t msgq;
	static char buf[sizeof(struct test_msg) * 2];

	ceell_msgq_init(&msgq, buf, sizeof(struct test_msg), 2);

	struct test_msg recv = { 0 };
	int ret = ceell_msgq_get(&msgq, &recv, CEELL_MSEC(10));

	zassert_not_equal(ret, 0, "get from empty queue should timeout");
}

ZTEST(osal_msgq, test_msgq_fifo_order)
{
	ceell_msgq_t msgq;
	static char buf[sizeof(struct test_msg) * 4];

	ceell_msgq_init(&msgq, buf, sizeof(struct test_msg), 4);

	for (uint32_t i = 1; i <= 3; i++) {
		struct test_msg msg = { .id = i, .data = i * 10 };

		ceell_msgq_put(&msgq, &msg, CEELL_NO_WAIT);
	}

	for (uint32_t i = 1; i <= 3; i++) {
		struct test_msg msg = { 0 };

		ceell_msgq_get(&msgq, &msg, CEELL_NO_WAIT);
		zassert_equal(msg.id, i, "FIFO order violated at %u", i);
	}
}

/* ── Time Tests ──────────────────────────────────────────────── */

ZTEST_SUITE(osal_time, NULL, NULL, NULL, NULL, NULL);

ZTEST(osal_time, test_uptime_monotonic)
{
	int64_t t1 = ceell_uptime_get();

	ceell_sleep(CEELL_MSEC(10));

	int64_t t2 = ceell_uptime_get();

	zassert_true(t2 > t1, "uptime should be monotonically increasing");
	zassert_true((t2 - t1) >= 10, "at least 10ms should have elapsed");
}

ZTEST(osal_time, test_sleep)
{
	int64_t start = ceell_uptime_get();

	ceell_sleep(CEELL_MSEC(50));

	int64_t elapsed = ceell_uptime_get() - start;

	zassert_true(elapsed >= 50, "sleep should wait at least 50ms");
	zassert_true(elapsed < 200, "sleep should not wait too long");
}

ZTEST(osal_time, test_cycle_counter)
{
	uint32_t c1 = ceell_cycle_get();
	uint32_t c2 = ceell_cycle_get();

	/* Cycle counter should advance (or at least not go backwards) */
	zassert_true(c2 >= c1, "cycle counter should not go backwards");
}

/* ── Atomic Tests ────────────────────────────────────────────── */

ZTEST_SUITE(osal_atomic, NULL, NULL, NULL, NULL, NULL);

ZTEST(osal_atomic, test_atomic_set_get)
{
	ceell_atomic_t val = CEELL_ATOMIC_INIT(0);

	ceell_atomic_set(&val, 42);
	zassert_equal(ceell_atomic_get(&val), 42, "atomic get should return set value");
}

ZTEST(osal_atomic, test_atomic_inc_dec)
{
	ceell_atomic_t val = CEELL_ATOMIC_INIT(10);

	int32_t result = ceell_atomic_inc(&val);

	zassert_equal(result, 11, "inc should return new value");
	zassert_equal(ceell_atomic_get(&val), 11, "value should be 11");

	result = ceell_atomic_dec(&val);
	zassert_equal(result, 10, "dec should return new value");
	zassert_equal(ceell_atomic_get(&val), 10, "value should be 10");
}

/* ── Thread Tests ────────────────────────────────────────────── */

ZTEST_SUITE(osal_thread, NULL, NULL, NULL, NULL, NULL);

static volatile bool thread_ran;

static void test_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	thread_ran = true;
	int *flag = (int *)p1;

	if (flag) {
		*flag = 99;
	}
}

CEELL_THREAD_STACK_DEFINE(test_stack, 1024);
static ceell_thread_t test_thread;

ZTEST(osal_thread, test_thread_create_and_run)
{
	int flag = 0;

	thread_ran = false;

	int ret = ceell_thread_create(&test_thread, test_stack,
				      CEELL_THREAD_STACK_SIZEOF(test_stack),
				      test_thread_entry, &flag, NULL, NULL, 7);

	zassert_equal(ret, 0, "thread create should succeed");

	/* Give the thread time to run */
	ceell_sleep(CEELL_MSEC(100));

	zassert_true(thread_ran, "thread should have run");
	zassert_equal(flag, 99, "thread should have set flag");
}
