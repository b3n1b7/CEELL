/*
 * CEELL Software Watchdog
 *
 * Provides a software watchdog timer that triggers safe state
 * if not fed within the configured timeout. Uses a background
 * thread to monitor the feed timestamp.
 */

#ifndef CEELL_WATCHDOG_H
#define CEELL_WATCHDOG_H

#include <stdbool.h>

/**
 * Initialize the software watchdog with the given timeout.
 * Starts a background monitoring thread.
 *
 * @param timeout_ms  Watchdog timeout in milliseconds. If ceell_watchdog_feed()
 *                    is not called within this interval, safe state is triggered.
 * @return 0 on success, negative errno on failure
 */
int ceell_watchdog_init(int timeout_ms);

/**
 * Feed (pet) the watchdog, resetting the timeout countdown.
 * Must be called periodically from the lifecycle loop.
 */
void ceell_watchdog_feed(void);

/**
 * Check whether the watchdog is currently active.
 *
 * @return true if the watchdog has been initialized and is running
 */
bool ceell_watchdog_enabled(void);

#endif /* CEELL_WATCHDOG_H */
