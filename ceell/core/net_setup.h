#ifndef CEELL_NET_SETUP_H
#define CEELL_NET_SETUP_H

#include "config_parser.h"

/**
 * Set up IPv4 address and netmask on the default network interface.
 * Must be called after ceell_config_load() and before any networking.
 *
 * @param cfg Parsed configuration with ip and netmask fields
 * @return 0 on success, negative errno on failure
 */
int ceell_net_setup(const struct ceell_config *cfg);

#endif /* CEELL_NET_SETUP_H */
