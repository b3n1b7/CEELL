#include <zephyr/kernel.h>
#include <string.h>

#include "config_parser.h"
#include "net_setup.h"
#include "node_identity.h"
#include "service_registry.h"
#include "discovery.h"
#include "messaging.h"
#include "lifecycle.h"

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
static int echo_handler(const char *payload, char *rsp_payload, size_t rsp_len)
{
	strncpy(rsp_payload, payload, rsp_len - 1);
	rsp_payload[rsp_len - 1] = '\0';
	return 0;
}
#endif

int main(void)
{
	struct ceell_config cfg;
	int ret;

	printk("CEELL middleware starting\n");

	/* Step 1: Load config from flash (or defaults) */
	ret = ceell_config_load(&cfg);
	if (ret < 0) {
		printk("CEELL: fatal config error (%d)\n", ret);
		return ret;
	}
	printk("CEELL: Config loaded (from_flash=%d)\n", cfg.from_flash);

	/* Step 2: Set IP + netmask on Ethernet interface */
	ret = ceell_net_setup(&cfg);
	if (ret < 0) {
		printk("CEELL: net setup failed (%d)\n", ret);
		return ret;
	}

	/* Step 3: Set node identity */
	ret = ceell_identity_init(&cfg);
	if (ret < 0 && ret != -EALREADY) {
		printk("CEELL: identity init failed (%d)\n", ret);
		return ret;
	}

	/* Step 4: Initialize service registry */
	ceell_service_init();

	/* Step 5: Register services from config */
	for (int i = 0; i < cfg.service_count; i++) {
		ret = ceell_service_register(cfg.services[i], NULL);
		if (ret < 0) {
			printk("CEELL: failed to register service '%s' (%d)\n",
			       cfg.services[i], ret);
		}
	}

#ifdef CONFIG_CEELL_TEST_ECHO_SERVICE
	ceell_service_register("echo", echo_handler);
#endif

	/* Step 6: Initialize discovery */
	ret = ceell_discovery_init();
	if (ret < 0) {
		printk("CEELL: discovery init failed (%d)\n", ret);
		return ret;
	}

	/* Step 7: Start discovery threads */
	ceell_discovery_start();
	printk("CEELL: Discovery started\n");

	/* Step 8: Initialize messaging */
	ret = ceell_messaging_init();
	if (ret < 0) {
		printk("CEELL: messaging init failed (%d)\n", ret);
		return ret;
	}

	/* Step 9: Lifecycle loop (never returns) */
	ceell_lifecycle_run();

	return 0;
}
