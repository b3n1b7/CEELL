/*
 * CEELL TSN Traffic Class Mapping — Implementation
 *
 * Stubbed API layer. Maps CEELL priorities to IEEE 802.1Q PCP values.
 * Socket priority setting is a no-op until TSN hardware (S32K5xx ENETC)
 * is available.
 */

#include "tsn.h"
#include "msg_priority.h"
#include "osal.h"

CEELL_LOG_MODULE_REGISTER(ceell_tsn, CEELL_LOG_LEVEL_INF);

/*
 * IEEE 802.1Q PCP (Priority Code Point) mapping table.
 *
 * Index = ceell_msg_priority enum value
 * Value = 802.1Q PCP (0-7)
 *
 * PCP 7 = Network Control (highest)
 * PCP 4 = Controlled Load
 * PCP 1 = Background (lowest used)
 */
static const int pcp_map[CEELL_MSG_PRIORITY_COUNT] = {
	[CEELL_MSG_CRITICAL] = 7,  /* Network Control */
	[CEELL_MSG_NORMAL]   = 4,  /* Controlled Load */
	[CEELL_MSG_BULK]     = 1,  /* Background */
};

int ceell_tsn_init(void)
{
	CEELL_LOG_INF("TSN subsystem initialized (stubbed — no hardware)");
	return 0;
}

int ceell_tsn_get_pcp(int ceell_priority)
{
	if (!ceell_msg_priority_valid(ceell_priority)) {
		return -1;
	}

	return pcp_map[ceell_priority];
}

int ceell_tsn_set_socket_priority(int sock, int ceell_priority)
{
	ARG_UNUSED(sock);

	if (!ceell_msg_priority_valid(ceell_priority)) {
		return -1;
	}

	if (!ceell_tsn_available()) {
		/* TSN not available — no-op, not an error */
		return 0;
	}

	/* When TSN hardware is present, this would call:
	 *   int pcp = pcp_map[ceell_priority];
	 *   ceell_setsockopt(sock, SOL_SOCKET, SO_PRIORITY,
	 *                    &pcp, sizeof(pcp));
	 */
	return 0;
}

bool ceell_tsn_available(void)
{
	/* No TSN hardware available yet.
	 * Will return true once S32K5xx ENETC driver is integrated. */
	return false;
}
