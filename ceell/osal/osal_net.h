/*
 * CEELL OSAL — Network Interface
 *
 * Wraps platform-specific network interface configuration (IP setup, etc.).
 * On Zephyr, wraps net_if_* and net_addr_* APIs.
 */

#ifndef CEELL_OSAL_NET_H
#define CEELL_OSAL_NET_H

#if defined(CONFIG_ZEPHYR)
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#endif

/**
 * Set IPv4 address and netmask on the default network interface.
 *
 * @param ip_str      IP address string (e.g. "192.168.1.10")
 * @param netmask_str Netmask string (e.g. "255.255.255.0"), may be empty
 * @return 0 on success, negative errno on failure
 */
int ceell_net_if_setup(const char *ip_str, const char *netmask_str);

#endif /* CEELL_OSAL_NET_H */
