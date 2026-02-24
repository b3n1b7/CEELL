#include "net_setup.h"

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

int ceell_net_setup(const struct ceell_config *cfg)
{
	struct net_if *iface;
	struct in_addr addr;
	struct in_addr mask;

	if (!cfg || cfg->ip[0] == '\0') {
		printk("CEELL: net_setup: no IP configured\n");
		return -EINVAL;
	}

	iface = net_if_get_default();
	if (!iface) {
		printk("CEELL: net_setup: no default interface\n");
		return -ENODEV;
	}

	if (net_addr_pton(AF_INET, cfg->ip, &addr) < 0) {
		printk("CEELL: net_setup: invalid IP '%s'\n", cfg->ip);
		return -EINVAL;
	}

	if (cfg->netmask[0] != '\0') {
		if (net_addr_pton(AF_INET, cfg->netmask, &mask) < 0) {
			printk("CEELL: net_setup: invalid netmask '%s'\n",
			       cfg->netmask);
			return -EINVAL;
		}
		net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
	}

	if (!net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0)) {
		printk("CEELL: net_setup: failed to add IP %s\n", cfg->ip);
		return -ENOBUFS;
	}

	printk("CEELL: net_setup: IP %s netmask %s\n", cfg->ip, cfg->netmask);
	return 0;
}
