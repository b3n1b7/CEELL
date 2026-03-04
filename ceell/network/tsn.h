/*
 * CEELL TSN Traffic Class Mapping (API layer, stubbed)
 *
 * Maps CEELL message priority levels to IEEE 802.1Q PCP
 * (Priority Code Point) values for future TSN hardware support.
 *
 * Behind CONFIG_CEELL_TSN (default n).
 */

#ifndef CEELL_TSN_H
#define CEELL_TSN_H

#include <stdbool.h>

/**
 * Initialize the TSN subsystem.
 *
 * Currently a no-op; will configure gPTP, CBS, and TAS gate
 * schedules once TSN-capable hardware (e.g. S32K5xx ENETC)
 * is available.
 *
 * @return 0 on success
 */
int ceell_tsn_init(void);

/**
 * Get the IEEE 802.1Q PCP value for a CEELL priority level.
 *
 * Mapping:
 *   CEELL_MSG_CRITICAL (0) -> PCP 7 (Network Control)
 *   CEELL_MSG_NORMAL   (1) -> PCP 4 (Controlled Load)
 *   CEELL_MSG_BULK     (2) -> PCP 1 (Background)
 *
 * @param ceell_priority  CEELL priority (CEELL_MSG_CRITICAL, etc.)
 * @return PCP value (0-7) on success, -1 on invalid priority
 */
int ceell_tsn_get_pcp(int ceell_priority);

/**
 * Set SO_PRIORITY on a socket based on CEELL priority.
 *
 * No-op if TSN hardware is not available.
 *
 * @param sock            Socket file descriptor
 * @param ceell_priority  CEELL priority level
 * @return 0 on success, -1 on error
 */
int ceell_tsn_set_socket_priority(int sock, int ceell_priority);

/**
 * Check whether TSN hardware is available.
 *
 * @return true if TSN is available, false otherwise
 */
bool ceell_tsn_available(void);

#endif /* CEELL_TSN_H */
