/*
 * CEELL OSAL — Zephyr Network Interface Implementation
 */

#include "../osal_net.h"
#include "../osal_log.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

int ceell_net_if_setup(const char *ip_str, const char *netmask_str)
{
	struct net_if *iface;
	struct in_addr addr;
	struct in_addr mask;

	if (!ip_str || ip_str[0] == '\0') {
		return -EINVAL;
	}

	iface = net_if_get_default();
	if (!iface) {
		return -ENODEV;
	}

	if (net_addr_pton(AF_INET, ip_str, &addr) < 0) {
		return -EINVAL;
	}

	if (netmask_str && netmask_str[0] != '\0') {
		if (net_addr_pton(AF_INET, netmask_str, &mask) < 0) {
			return -EINVAL;
		}
		net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
	}

	if (!net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0)) {
		return -ENOBUFS;
	}

	return 0;
}
