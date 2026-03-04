/*
 * CEELL Software Watchdog — Implementation
 *
 * A background thread periodically checks whether the watchdog
 * has been fed within the timeout window. If not, it triggers
 * safe state via ceell_safe_state_trigger().
 */

#include "watchdog.h"
#include "safe_state.h"
#include "osal.h"

#include <errno.h>

CEELL_LOG_MODULE_REGISTER(ceell_watchdog, CEELL_LOG_LEVEL_INF);

#define WDT_THREAD_STACK_SIZE 1024
#define WDT_THREAD_PRIORITY   10
#define WDT_CHECK_INTERVAL_MS 500

CEELL_THREAD_STACK_DEFINE(ceell_wdt_stack, WDT_THREAD_STACK_SIZE);
static ceell_thread_t wdt_thread_data;

static ceell_atomic_t wdt_enabled = CEELL_ATOMIC_INIT(0);
static ceell_atomic_t wdt_last_feed = CEELL_ATOMIC_INIT(0);
static int wdt_timeout;

static void wdt_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (ceell_atomic_get(&wdt_enabled) != 0) {
		int64_t now = ceell_uptime_get();
		int32_t last = ceell_atomic_get(&wdt_last_feed);
		int64_t elapsed = now - (int64_t)last;

		if (elapsed > (int64_t)wdt_timeout) {
			CEELL_LOG_ERR("Watchdog expired! elapsed=%lld timeout=%d",
				      (long long)elapsed, wdt_timeout);
			ceell_safe_state_trigger(CEELL_SAFE_COMM_FAILURE);

			/*
			 * After triggering, keep monitoring — the watchdog
			 * can recover if feed() is called and safe state
			 * is cleared externally.
			 */
		}

		ceell_sleep(CEELL_MSEC(WDT_CHECK_INTERVAL_MS));
	}
}

int ceell_watchdog_init(int timeout_ms)
{
	if (timeout_ms <= 0) {
		return -EINVAL;
	}

	wdt_timeout = timeout_ms;

	/* Set initial feed time to now */
	ceell_atomic_set(&wdt_last_feed, (int32_t)ceell_uptime_get());
	ceell_atomic_set(&wdt_enabled, 1);

	int ret = ceell_thread_create(&wdt_thread_data, ceell_wdt_stack,
				      CEELL_THREAD_STACK_SIZEOF(ceell_wdt_stack),
				      wdt_thread_fn, NULL, NULL, NULL,
				      WDT_THREAD_PRIORITY);
	if (ret < 0) {
		ceell_atomic_set(&wdt_enabled, 0);
		CEELL_LOG_ERR("Watchdog thread create failed (%d)", ret);
		return ret;
	}

	ceell_thread_name_set(&wdt_thread_data, "ceell_wdt");

	CEELL_LOG_INF("Software watchdog initialized (timeout=%d ms)", timeout_ms);
	return 0;
}

void ceell_watchdog_feed(void)
{
	if (ceell_atomic_get(&wdt_enabled) != 0) {
		ceell_atomic_set(&wdt_last_feed, (int32_t)ceell_uptime_get());
	}
}

bool ceell_watchdog_enabled(void)
{
	return ceell_atomic_get(&wdt_enabled) != 0;
}
