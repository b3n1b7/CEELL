#include "lifecycle.h"
#include "node_identity.h"
#include "discovery.h"
#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
#include "messaging.h"
#include "service_discovery.h"
#endif

#include <zephyr/kernel.h>

#define LIFECYCLE_INTERVAL_SEC 5

void ceell_lifecycle_run(void)
{
	const struct ceell_config *id = ceell_identity_get();
#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
	bool echo_tested = false;
#endif

	if (!id) {
		printk("CEELL: lifecycle — identity not set\n");
		return;
	}

	while (1) {
		ceell_discovery_expire_peers();

		int peers = ceell_discovery_peer_count();

		printk("CEELL_HEALTH node_id=%u peers=%d\n",
		       id->node_id, peers);

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
		/* Once we have peers, send one echo test request */
		if (!echo_tested && peers > 0) {
			char peer_ip[16];

			if (ceell_service_find_peer("echo", peer_ip) == 0) {
				struct ceell_msg rsp;

				printk("CEELL_ECHO_TEST sending to %s\n",
				       peer_ip);
				int ret = ceell_msg_send("echo", "hello-ceell",
							 &rsp,
							 K_SECONDS(3));
				if (ret == 0) {
					printk("CEELL_ECHO_TEST_OK status=%d payload=%s\n",
					       rsp.status, rsp.payload);
				} else {
					printk("CEELL_ECHO_TEST_FAIL err=%d\n",
					       ret);
				}
				echo_tested = true;
			}
		}
#endif

		k_sleep(K_SECONDS(LIFECYCLE_INTERVAL_SEC));
	}
}
