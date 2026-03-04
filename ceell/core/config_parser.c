#include "config_parser.h"

#include <string.h>
#include "osal.h"
#include "osal_json.h"
#include "osal_flash.h"

#define CONFIG_PARTITION_ID CEELL_FIXED_PARTITION_ID(config_partition)
#define CONFIG_READ_SIZE    512

/* ── V1 JSON descriptor (CSV services string) ─────────────────── */
struct json_config_v1 {
	int version;
	int node_id;
	const char *role;
	const char *name;
	const char *ip;
	const char *netmask;
	const char *services;
};

static const struct json_obj_descr json_config_v1_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, node_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, role, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, ip, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, netmask, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v1, services, JSON_TOK_STRING),
};

/* ── V2 JSON descriptors (services array with SLA) ─────────────── */
struct json_svc_entry {
	const char *name;
	int priority;
	int deadline_ms;
};

static const struct json_obj_descr json_svc_entry_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_svc_entry, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_svc_entry, priority, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_svc_entry, deadline_ms,
			     JSON_TOK_NUMBER),
};

struct json_config_v2 {
	int version;
	int node_id;
	const char *role;
	const char *name;
	const char *ip;
	const char *netmask;
	struct json_svc_entry services[CEELL_CONFIG_MAX_SERVICES];
	size_t services_len;
};

static const struct json_obj_descr json_config_v2_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, node_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, role, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, ip, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_config_v2, netmask, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_config_v2, services,
				  CEELL_CONFIG_MAX_SERVICES, services_len,
				  json_svc_entry_descr,
				  ARRAY_SIZE(json_svc_entry_descr)),
};

/**
 * Parse comma-separated service names into cfg->services[] (V1 format).
 * E.g. "body-control,window-lift" -> services[0]="body-control",
 *                                    services[1]="window-lift"
 * SLAs default to NORMAL priority, no deadline.
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
			int idx = cfg->service_count;

			memcpy(cfg->services[idx], p, len);
			cfg->services[idx][len] = '\0';

			/* Default SLA for v1: NORMAL priority, no deadline */
			strncpy(cfg->service_slas[idx].name,
				cfg->services[idx],
				CEELL_CONFIG_MAX_SVC_NAME_LEN - 1);
			cfg->service_slas[idx].priority = 1; /* NORMAL */
			cfg->service_slas[idx].deadline_ms = 0;

			cfg->service_count++;
		}

		if (comma) {
			p = comma + 1;
		} else {
			break;
		}
	}
}

/**
 * Parse V2 services array with SLA data into cfg.
 */
static void parse_services_v2(struct ceell_config *cfg,
			      const struct json_config_v2 *jcfg)
{
	cfg->service_count = 0;

	for (size_t i = 0; i < jcfg->services_len &&
	     cfg->service_count < CEELL_CONFIG_MAX_SERVICES; i++) {
		const struct json_svc_entry *entry = &jcfg->services[i];

		if (!entry->name || entry->name[0] == '\0') {
			continue;
		}

		int idx = cfg->service_count;

		strncpy(cfg->services[idx], entry->name,
			CEELL_CONFIG_MAX_SVC_NAME_LEN - 1);
		cfg->services[idx][CEELL_CONFIG_MAX_SVC_NAME_LEN - 1] = '\0';

		strncpy(cfg->service_slas[idx].name, entry->name,
			CEELL_CONFIG_MAX_SVC_NAME_LEN - 1);
		cfg->service_slas[idx].name[
			CEELL_CONFIG_MAX_SVC_NAME_LEN - 1] = '\0';

		/* Clamp priority to valid range */
		if (entry->priority >= 0 && entry->priority <= 2) {
			cfg->service_slas[idx].priority =
				(uint8_t)entry->priority;
		} else {
			cfg->service_slas[idx].priority = 1; /* NORMAL */
		}

		if (entry->deadline_ms >= 0) {
			cfg->service_slas[idx].deadline_ms =
				(uint16_t)entry->deadline_ms;
		} else {
			cfg->service_slas[idx].deadline_ms = 0;
		}

		cfg->service_count++;
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

/**
 * Copy common fields (node_id, role, name, ip, netmask) from parsed JSON
 * into the config struct.
 */
static void copy_common_fields(struct ceell_config *cfg, int node_id,
			       const char *role, const char *name,
			       const char *ip, const char *netmask)
{
	cfg->node_id = (uint32_t)node_id;
	cfg->from_flash = true;

	if (role) {
		strncpy(cfg->role, role, CEELL_CONFIG_MAX_ROLE_LEN - 1);
	}
	if (name) {
		strncpy(cfg->name, name, CEELL_CONFIG_MAX_NAME_LEN - 1);
	}
	if (ip) {
		strncpy(cfg->ip, ip, CEELL_CONFIG_MAX_IP_LEN - 1);
	}
	if (netmask) {
		strncpy(cfg->netmask, netmask, CEELL_CONFIG_MAX_IP_LEN - 1);
	}
}

/**
 * Try to detect the version field from JSON without full parse.
 * Parses only the "version" field.
 */
struct json_version_only {
	int version;
};

static const struct json_obj_descr json_version_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_version_only, version,
			     JSON_TOK_NUMBER),
};

int ceell_config_load(struct ceell_config *cfg)
{
	static uint8_t read_buf[CONFIG_READ_SIZE];
	ceell_flash_area_t *fa;
	int ret;

	memset(cfg, 0, sizeof(*cfg));

	ret = ceell_flash_open(CONFIG_PARTITION_ID, &fa);
	if (ret < 0) {
		/*
		 * Flash device not available (e.g. Renode without QSPI model).
		 * Try memory-mapped fallback: config binary loaded at
		 * 0x68000000 via Renode's "sysbus LoadBinary" command.
		 */
		const uint8_t *mapped = (const uint8_t *)0x68000000;

		if (!flash_is_empty(mapped, CONFIG_READ_SIZE)) {
			ceell_printk("CEELL: flash unavailable, using "
				     "memory-mapped config\n");
			memcpy(read_buf, mapped, CONFIG_READ_SIZE);
			goto parse_json;
		}
		ceell_printk("CEELL: flash open failed (%d), using "
			     "defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	ret = ceell_flash_read(fa, 0, read_buf, CONFIG_READ_SIZE);
	ceell_flash_close(fa);

	if (ret < 0) {
		ceell_printk("CEELL: flash read failed (%d), using "
			     "defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	if (flash_is_empty(read_buf, CONFIG_READ_SIZE)) {
		ceell_printk("CEELL: flash config empty, using defaults\n");
		load_defaults(cfg);
		return 0;
	}

parse_json:
	/* Null-terminate the JSON buffer */
	read_buf[CONFIG_READ_SIZE - 1] = '\0';

	/* First, detect config version */
	struct json_version_only ver_only = { 0 };

	ret = json_obj_parse((char *)read_buf, strlen((char *)read_buf),
			     json_version_descr,
			     ARRAY_SIZE(json_version_descr),
			     &ver_only);
	if (ret < 0) {
		ceell_printk("CEELL: JSON version parse failed (%d), using "
			     "defaults\n", ret);
		load_defaults(cfg);
		return 0;
	}

	if (ver_only.version == CEELL_CONFIG_VERSION_V2) {
		/* Parse V2 format (services array with SLA) */
		struct json_config_v2 jcfg = { 0 };

		ret = json_obj_parse((char *)read_buf,
				     strlen((char *)read_buf),
				     json_config_v2_descr,
				     ARRAY_SIZE(json_config_v2_descr),
				     &jcfg);
		if (ret < 0) {
			ceell_printk("CEELL: V2 JSON parse failed (%d), "
				     "using defaults\n", ret);
			load_defaults(cfg);
			return 0;
		}

		cfg->version = (uint32_t)jcfg.version;
		copy_common_fields(cfg, jcfg.node_id, jcfg.role, jcfg.name,
				   jcfg.ip, jcfg.netmask);
		parse_services_v2(cfg, &jcfg);

		ceell_printk("CEELL: Config V2 loaded from flash: "
			     "node_id=%u role=%s name=%s ip=%s "
			     "services=%d\n",
			     cfg->node_id, cfg->role, cfg->name,
			     cfg->ip, cfg->service_count);
		for (int i = 0; i < cfg->service_count; i++) {
			ceell_printk("CEELL:   svc[%d]=%s pri=%u "
				     "deadline=%u\n",
				     i, cfg->service_slas[i].name,
				     cfg->service_slas[i].priority,
				     cfg->service_slas[i].deadline_ms);
		}
		return 0;
	}

	if (ver_only.version == CEELL_CONFIG_VERSION_V1) {
		/* Parse V1 format (CSV services string) */
		struct json_config_v1 jcfg = { 0 };

		ret = json_obj_parse((char *)read_buf,
				     strlen((char *)read_buf),
				     json_config_v1_descr,
				     ARRAY_SIZE(json_config_v1_descr),
				     &jcfg);
		if (ret < 0) {
			ceell_printk("CEELL: V1 JSON parse failed (%d), "
				     "using defaults\n", ret);
			load_defaults(cfg);
			return 0;
		}

		cfg->version = (uint32_t)jcfg.version;
		copy_common_fields(cfg, jcfg.node_id, jcfg.role, jcfg.name,
				   jcfg.ip, jcfg.netmask);
		parse_services_csv(cfg, jcfg.services);

		ceell_printk("CEELL: Config V1 loaded from flash: "
			     "version=%u node_id=%u role=%s name=%s "
			     "ip=%s netmask=%s services=%d\n",
			     cfg->version, cfg->node_id, cfg->role,
			     cfg->name, cfg->ip, cfg->netmask,
			     cfg->service_count);
		return 0;
	}

	ceell_printk("CEELL: unsupported config version %d, using "
		     "defaults\n", ver_only.version);
	load_defaults(cfg);
	return 0;
}
