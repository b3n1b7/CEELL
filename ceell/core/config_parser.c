#include "config_parser.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <zephyr/storage/flash_map.h>

#define CONFIG_PARTITION_ID FIXED_PARTITION_ID(config_partition)
#define CONFIG_READ_SIZE    512

/* JSON descriptor for parsing */
struct json_config {
	int version;
	int node_id;
	const char *role;
	const char *name;
	const char *ip;
	const char *netmask;
	const char *services;
};

static const struct json_obj_descr json_config_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_config, version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config, node_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config, role, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config, ip, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config, netmask, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config, services, JSON_TOK_STRING),
};

/**
 * Parse comma-separated service names into cfg->services[].
 * E.g. "body-control,window-lift" → services[0]="body-control", services[1]="window-lift"
 */
static void parse_services_csv(struct ceell_config *cfg, const char *csv)
{
	cfg->service_count = 0;

	if (!csv || csv[0] == '\0') {
		return;
	}

	const char *p = csv;

	while (*p && cfg->service_count < CEELL_CONFIG_MAX_SERVICES) {
		const char *comma = strchr(p, ',');
		size_t len;

		if (comma) {
			len = comma - p;
		} else {
			len = strlen(p);
		}

		if (len > 0 && len < CEELL_CONFIG_MAX_SVC_NAME_LEN) {
			memcpy(cfg->services[cfg->service_count], p, len);
			cfg->services[cfg->service_count][len] = '\0';
			cfg->service_count++;
		}

		if (comma) {
			p = comma + 1;
		} else {
			break;
		}
	}
}

static void load_defaults(struct ceell_config *cfg)
{
	cfg->version = 0;
	cfg->node_id = CONFIG_CEELL_NODE_ID_DEFAULT;
	strncpy(cfg->role, CONFIG_CEELL_NODE_ROLE_DEFAULT,
		CEELL_CONFIG_MAX_ROLE_LEN - 1);
	cfg->role[CEELL_CONFIG_MAX_ROLE_LEN - 1] = '\0';
	strncpy(cfg->name, CONFIG_CEELL_NODE_NAME_DEFAULT,
		CEELL_CONFIG_MAX_NAME_LEN - 1);
	cfg->name[CEELL_CONFIG_MAX_NAME_LEN - 1] = '\0';
	strncpy(cfg->ip, CONFIG_CEELL_NODE_IP_DEFAULT,
		CEELL_CONFIG_MAX_IP_LEN - 1);
	cfg->ip[CEELL_CONFIG_MAX_IP_LEN - 1] = '\0';
	strncpy(cfg->netmask, CONFIG_CEELL_NODE_NETMASK_DEFAULT,
		CEELL_CONFIG_MAX_IP_LEN - 1);
	cfg->netmask[CEELL_CONFIG_MAX_IP_LEN - 1] = '\0';
	cfg->service_count = 0;
	cfg->from_flash = false;
}

static bool flash_is_empty(const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != 0xFF) {
			return false;
		}
	}
	return true;
}

int ceell_config_load(struct ceell_config *cfg)
{
	static uint8_t read_buf[CONFIG_READ_SIZE];
	const struct flash_area *fa;
	int ret;

	memset(cfg, 0, sizeof(*cfg));

	ret = flash_area_open(CONFIG_PARTITION_ID, &fa);
	if (ret < 0) {
		/*
		 * Flash device not available (e.g. Renode without QSPI model).
		 * Try memory-mapped fallback: config binary loaded at 0x68000000
		 * via Renode's "sysbus LoadBinary" command.
		 */
		const uint8_t *mapped = (const uint8_t *)0x68000000;

		if (!flash_is_empty(mapped, CONFIG_READ_SIZE)) {
			printk("CEELL: flash unavailable, using memory-mapped config\n");
			memcpy(read_buf, mapped, CONFIG_READ_SIZE);
			goto parse_json;
		}
		printk("CEELL: flash open failed (%d), using defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	ret = flash_area_read(fa, 0, read_buf, CONFIG_READ_SIZE);
	flash_area_close(fa);

	if (ret < 0) {
		printk("CEELL: flash read failed (%d), using defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	if (flash_is_empty(read_buf, CONFIG_READ_SIZE)) {
		printk("CEELL: flash config empty, using defaults\n");
		load_defaults(cfg);
		return 0;
	}

parse_json:
	/* Null-terminate the JSON buffer */
	read_buf[CONFIG_READ_SIZE - 1] = '\0';

	struct json_config jcfg = { 0 };

	ret = json_obj_parse((char *)read_buf, strlen((char *)read_buf),
			     json_config_descr,
			     ARRAY_SIZE(json_config_descr),
			     &jcfg);
	if (ret < 0) {
		printk("CEELL: JSON parse failed (%d), using defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	/* Validate version */
	if (jcfg.version != CEELL_CONFIG_VERSION) {
		printk("CEELL: unsupported config version %d (expected %d), using defaults\n",
		       jcfg.version, CEELL_CONFIG_VERSION);
		load_defaults(cfg);
		return 0;
	}

	cfg->version = (uint32_t)jcfg.version;
	cfg->node_id = (uint32_t)jcfg.node_id;
	cfg->from_flash = true;

	if (jcfg.role) {
		strncpy(cfg->role, jcfg.role, CEELL_CONFIG_MAX_ROLE_LEN - 1);
	}
	if (jcfg.name) {
		strncpy(cfg->name, jcfg.name, CEELL_CONFIG_MAX_NAME_LEN - 1);
	}
	if (jcfg.ip) {
		strncpy(cfg->ip, jcfg.ip, CEELL_CONFIG_MAX_IP_LEN - 1);
	}
	if (jcfg.netmask) {
		strncpy(cfg->netmask, jcfg.netmask, CEELL_CONFIG_MAX_IP_LEN - 1);
	}

	parse_services_csv(cfg, jcfg.services);

	printk("CEELL: Config loaded from flash: version=%u node_id=%u role=%s name=%s ip=%s netmask=%s services=%d\n",
	       cfg->version, cfg->node_id, cfg->role, cfg->name,
	       cfg->ip, cfg->netmask, cfg->service_count);
	return 0;
}
