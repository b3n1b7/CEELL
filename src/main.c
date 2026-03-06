/* REQ-SAF-001 */

#include <string.h>

#include "osal_log.h"
#include "config_parser.h"
#include "net_setup.h"
#include "node_identity.h"
#include "service_registry.h"
#include "discovery.h"
#include "messaging.h"
#include "lifecycle.h"
#include "health_monitor.h"
#include "safe_state.h"
#include "timing.h"

#ifdef CONFIG_CEELL_OTA
#include "ota_client.h"
#endif

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
static int echo_handler(const char *payload, char *rsp_payload, size_t rsp_len)
{
	strncpy(rsp_payload, payload, rsp_len - 1);
	rsp_payload[rsp_len - 1] = '\0';
	return 0;
}
#endif

static void on_health_change(uint32_t node_id, int old_state, int new_state)
{
	ceell_printk("CEELL: Peer %u health: %d -> %d\n",
		     node_id, old_state, new_state);

	if (new_state == CEELL_HEALTH_LOST) {
		ceell_safe_state_trigger(CEELL_SAFE_CRITICAL_PEER_LOST);
		ceell_printk("CEELL: SAFE STATE TRIGGERED peer=%u\n", node_id);
	} else if (old_state == CEELL_HEALTH_LOST &&
		   new_state == CEELL_HEALTH_HEALTHY) {
		ceell_safe_state_clear();
		ceell_printk("CEELL: SAFE STATE CLEARED peer=%u\n", node_id);
	}
}

int main(void)
{
	struct ceell_config cfg;
	int ret;

	ceell_printk("CEELL middleware starting\n");

	/* Step 1: Load config from flash (or defaults) */
	ret = ceell_config_load(&cfg);
	if (ret < 0) {
		ceell_printk("CEELL: fatal config error (%d)\n", ret);
		return ret;
	}
	ceell_printk("CEELL: Config loaded (from_flash=%d version=%u)\n",
		     cfg.from_flash, cfg.version);

	/* Step 2: Set IP + netmask on Ethernet interface */
	ret = ceell_net_setup(&cfg);
	if (ret < 0) {
		ceell_printk("CEELL: net setup failed (%d)\n", ret);
		return ret;
	}

	/* Step 3: Set node identity */
	ret = ceell_identity_init(&cfg);
	if (ret < 0 && ret != -EALREADY) {
		ceell_printk("CEELL: identity init failed (%d)\n", ret);
		return ret;
	}

	/* Step 4: Initialize service registry */
	ceell_service_init();

	/* Step 5: Register services from config with SLA data */
	for (int i = 0; i < cfg.service_count; i++) {
		if (cfg.version == CEELL_CONFIG_VERSION_V2) {
			/* V2: register with per-service SLA */
			ret = ceell_service_register_sla(
				cfg.services[i], NULL,
				cfg.service_slas[i].priority,
				cfg.service_slas[i].deadline_ms);
		} else {
			/* V1 or defaults: register with default SLA */
			ret = ceell_service_register(cfg.services[i], NULL);
		}
		if (ret < 0) {
			ceell_printk("CEELL: failed to register service "
				     "'%s' (%d)\n", cfg.services[i], ret);
		}
	}

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
	ceell_service_register("echo", echo_handler);
#endif

	/* Step 5b: Initialize timing subsystem */
	ceell_timing_init();
	ceell_printk("CEELL: Timing subsystem initialized\n");

	/* Step 5c: Initialize health monitor */
	ret = ceell_health_init();
	if (ret < 0) {
		ceell_printk("CEELL: health init failed (%d)\n", ret);
	} else {
		ceell_health_register_cb(on_health_change);
		ceell_printk("CEELL: Health monitor initialized\n");
	}

	/* Step 5d: Initialize safe state manager */
	ret = ceell_safe_state_init();
	if (ret < 0) {
		ceell_printk("CEELL: safe state init failed (%d)\n", ret);
	} else {
		ceell_printk("CEELL: Safe state manager initialized\n");
	}

	/* Step 6: Initialize discovery */
	ret = ceell_discovery_init();
	if (ret < 0) {
		ceell_printk("CEELL: discovery init failed (%d)\n", ret);
		return ret;
	}

	/* Step 7: Start discovery threads */
	ceell_discovery_start();
	ceell_printk("CEELL: Discovery started\n");

	/* Step 8: Initialize messaging */
	ret = ceell_messaging_init();
	if (ret < 0) {
		ceell_printk("CEELL: messaging init failed (%d)\n", ret);
		return ret;
	}

#ifdef CONFIG_CEELL_OTA
	/* Step 9: Register OTA service + start OTA thread */
	ceell_service_register("ota", ceell_ota_service_handler);

	ret = ceell_ota_init();
	if (ret < 0) {
		ceell_printk("CEELL: OTA init failed (%d)\n", ret);
		/* Non-fatal: node runs without OTA */
	} else {
		ceell_printk("CEELL: OTA initialized\n");
	}
#endif

	/* Step 10: Lifecycle loop (never returns) */
	ceell_lifecycle_run();

	return 0;
}
