/* REQ-SAF-001 */ /* REQ-SAF-002 */

#include "lifecycle.h"
#include "node_identity.h"
#include "discovery.h"
#include "health_monitor.h"
#include "timing.h"
#ifdef CONFIG_CEELL_WATCHDOG
#include "watchdog.h"
#endif
#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
#include "messaging.h"
#include "service_discovery.h"
#endif

#include "osal.h"

#define LIFECYCLE_INTERVAL_SEC 5
#define LIFECYCLE_STATS_INTERVAL 6

void ceell_lifecycle_run(void)
{
	const struct ceell_config *id = ceell_identity_get();
#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
	bool echo_tested = false;
#endif
	static int tick_count;

	if (!id) {
		ceell_printk("CEELL: lifecycle — identity not set\n");
		return;
	}

	while (1) {
		uint32_t th;

		ceell_timing_start("lifecycle_tick", &th);
		ceell_discovery_expire_peers();
		ceell_health_update();
		ceell_timing_stop("lifecycle_tick", th);

#ifdef CONFIG_CEELL_WATCHDOG
		ceell_watchdog_feed();
#endif

		if (++tick_count % LIFECYCLE_STATS_INTERVAL == 0) {
			ceell_timing_print_stats();
		}

		int peers = ceell_discovery_peer_count();

		ceell_printk("CEELL_HEALTH node_id=%u peers=%d\n",
			     id->node_id, peers);

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
		/* Once we have peers, send echo test (retries until success) */
		if (!echo_tested && peers > 0) {
			char peer_ip[16];

			if (ceell_service_find_peer("echo", peer_ip) == 0) {
				struct ceell_msg rsp;

				ceell_printk("CEELL_ECHO_TEST sending to %s\n",
					     peer_ip);
				int ret = ceell_msg_send("echo", "hello-ceell",
							 &rsp,
							 CEELL_SECONDS(5));
				if (ret == 0) {
					ceell_printk("CEELL_ECHO_TEST_OK status=%d payload=%s\n",
						     rsp.status, rsp.payload);
					echo_tested = true;
				} else {
					ceell_printk("CEELL_ECHO_TEST_FAIL err=%d (will retry)\n",
						     ret);
				}
			}
		}
#endif

		ceell_sleep(CEELL_SECONDS(LIFECYCLE_INTERVAL_SEC));
	}
}
