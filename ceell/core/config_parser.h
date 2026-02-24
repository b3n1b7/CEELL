#ifndef CEELL_CONFIG_PARSER_H
#define CEELL_CONFIG_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define CEELL_CONFIG_MAX_ROLE_LEN     32
#define CEELL_CONFIG_MAX_NAME_LEN     64
#define CEELL_CONFIG_MAX_IP_LEN       16
#define CEELL_CONFIG_MAX_SERVICES     4
#define CEELL_CONFIG_MAX_SVC_NAME_LEN 32

#define CEELL_CONFIG_VERSION  1

struct ceell_config {
	uint32_t version;
	uint32_t node_id;
	char role[CEELL_CONFIG_MAX_ROLE_LEN];
	char name[CEELL_CONFIG_MAX_NAME_LEN];
	char ip[CEELL_CONFIG_MAX_IP_LEN];
	char netmask[CEELL_CONFIG_MAX_IP_LEN];
	char services[CEELL_CONFIG_MAX_SERVICES][CEELL_CONFIG_MAX_SVC_NAME_LEN];
	int service_count;
	bool from_flash;
};

/**
 * Load config from flash partition. Falls back to Kconfig defaults
 * if flash is empty (all 0xFF), parse fails, or version is unsupported.
 *
 * @param cfg Output config struct
 * @return 0 on success (even if using defaults), negative errno on fatal error
 */
int ceell_config_load(struct ceell_config *cfg);

#endif /* CEELL_CONFIG_PARSER_H */
