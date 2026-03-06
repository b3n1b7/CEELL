/*
 * Unit tests for CEELL Config Parser
 *
 * Includes config_parser.c directly to access static functions.
 * Flash functions are mocked via stubs controlled by test globals.
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <errno.h>

/* Block osal_flash.h and osal_socket.h — we mock flash and don't need sockets */
#define CEELL_OSAL_FLASH_H
#define CEELL_OSAL_SOCKET_H

/* Provide our own flash types before osal.h is included */
typedef int ceell_flash_area_t;
#define CEELL_FIXED_PARTITION_ID(x) 0

/* ── Flash mock state ───────────────────────────────────────────── */

static uint8_t test_flash_buf[512];
static bool test_flash_open_fail;
static bool test_flash_read_fail;

int ceell_flash_open(uint8_t id, ceell_flash_area_t **fa)
{
	ARG_UNUSED(id);

	if (test_flash_open_fail) {
		return -EIO;
	}
	*fa = (ceell_flash_area_t *)1; /* non-NULL sentinel */
	return 0;
}

int ceell_flash_read(ceell_flash_area_t *fa, uint32_t off,
		     void *dst, size_t len)
{
	ARG_UNUSED(fa);

	if (test_flash_read_fail) {
		return -EIO;
	}
	memcpy(dst, test_flash_buf + off, len);
	return 0;
}

void ceell_flash_close(ceell_flash_area_t *fa)
{
	ARG_UNUSED(fa);
}

/* Include config_parser.c directly to access static helpers */
#include "config_parser.c"

/* ── Test fixture ───────────────────────────────────────────────── */

static void setup_flash(const char *json)
{
	memset(test_flash_buf, 0xFF, sizeof(test_flash_buf));
	test_flash_open_fail = false;
	test_flash_read_fail = false;

	if (json) {
		size_t len = strlen(json);

		if (len >= sizeof(test_flash_buf)) {
			len = sizeof(test_flash_buf) - 1;
		}
		memcpy(test_flash_buf, json, len);
		test_flash_buf[len] = '\0';
	}
}

static void config_before(void *fixture)
{
	ARG_UNUSED(fixture);
	setup_flash(NULL);
}

ZTEST_SUITE(config_parser, NULL, NULL, config_before, NULL, NULL);

/* ── Flash failure tests ────────────────────────────────────────── */

ZTEST(config_parser, test_flash_empty)
{
	struct ceell_config cfg;

	/* Flash is all 0xFF (default after config_before) */
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0, "should succeed with defaults");
	zassert_false(cfg.from_flash, "should not be from flash");
	zassert_equal(cfg.node_id, 0, "default node_id should be 0");
	zassert_str_equal(cfg.role, "unknown", "default role");
	zassert_str_equal(cfg.name, "ceell-unprovisioned", "default name");
	zassert_str_equal(cfg.ip, "192.168.1.100", "default ip");
	zassert_str_equal(cfg.netmask, "255.255.255.0", "default netmask");
	zassert_equal(cfg.service_count, 0, "no services by default");
}

ZTEST(config_parser, test_flash_read_fails)
{
	struct ceell_config cfg;

	test_flash_read_fail = true;
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0, "should succeed with defaults on read error");
	zassert_false(cfg.from_flash);
}

/* ── V1 format tests ───────────────────────────────────────────── */

ZTEST(config_parser, test_v1_basic)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":1,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\",\"services\":\"braking\"}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_true(cfg.from_flash);
	zassert_equal(cfg.version, 1);
	zassert_equal(cfg.node_id, 1);
	zassert_str_equal(cfg.role, "body");
	zassert_str_equal(cfg.name, "body-ecu");
	zassert_str_equal(cfg.ip, "192.168.1.10");
	zassert_str_equal(cfg.netmask, "255.255.255.0");
	zassert_equal(cfg.service_count, 1);
	zassert_str_equal(cfg.services[0], "braking");
	/* V1 defaults: NORMAL priority, no deadline */
	zassert_equal(cfg.service_slas[0].priority, 1);
	zassert_equal(cfg.service_slas[0].deadline_ms, 0);
}

ZTEST(config_parser, test_v1_multiple_services)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":1,\"node_id\":2,\"role\":\"chassis\","
		"\"name\":\"chassis-ecu\",\"ip\":\"192.168.1.11\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":\"braking,steering,lighting\"}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, 3);
	zassert_str_equal(cfg.services[0], "braking");
	zassert_str_equal(cfg.services[1], "steering");
	zassert_str_equal(cfg.services[2], "lighting");
}

ZTEST(config_parser, test_v1_empty_services)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":1,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\",\"services\":\"\"}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, 0, "empty CSV -> 0 services");
}

ZTEST(config_parser, test_v1_max_services)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":1,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":\"svc1,svc2,svc3,svc4\"}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, CEELL_CONFIG_MAX_SERVICES,
		      "should parse exactly max services");
	zassert_str_equal(cfg.services[0], "svc1");
	zassert_str_equal(cfg.services[3], "svc4");
}

/* ── V2 format tests ───────────────────────────────────────────── */

ZTEST(config_parser, test_v2_basic)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":2,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":[{\"name\":\"braking\","
		"\"priority\":0,\"deadline_ms\":50}]}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_true(cfg.from_flash);
	zassert_equal(cfg.version, 2);
	zassert_equal(cfg.service_count, 1);
	zassert_str_equal(cfg.services[0], "braking");
	zassert_equal(cfg.service_slas[0].priority, 0, "CRITICAL priority");
	zassert_equal(cfg.service_slas[0].deadline_ms, 50);
}

ZTEST(config_parser, test_v2_multiple_services)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":2,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":["
		"{\"name\":\"braking\",\"priority\":0,\"deadline_ms\":50},"
		"{\"name\":\"steering\",\"priority\":1,\"deadline_ms\":100},"
		"{\"name\":\"lighting\",\"priority\":2,\"deadline_ms\":0}"
		"]}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, 3);
	zassert_str_equal(cfg.services[0], "braking");
	zassert_equal(cfg.service_slas[0].priority, 0);
	zassert_equal(cfg.service_slas[0].deadline_ms, 50);
	zassert_str_equal(cfg.services[1], "steering");
	zassert_equal(cfg.service_slas[1].priority, 1);
	zassert_equal(cfg.service_slas[1].deadline_ms, 100);
	zassert_str_equal(cfg.services[2], "lighting");
	zassert_equal(cfg.service_slas[2].priority, 2);
	zassert_equal(cfg.service_slas[2].deadline_ms, 0);
}

ZTEST(config_parser, test_v2_priority_clamping)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":2,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":[{\"name\":\"test\","
		"\"priority\":99,\"deadline_ms\":0}]}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, 1);
	zassert_equal(cfg.service_slas[0].priority, 1,
		      "out-of-range priority should clamp to NORMAL (1)");
}

ZTEST(config_parser, test_v2_empty_service_name)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":2,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\","
		"\"services\":["
		"{\"name\":\"\",\"priority\":0,\"deadline_ms\":50},"
		"{\"name\":\"valid\",\"priority\":1,\"deadline_ms\":0}"
		"]}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0);
	zassert_equal(cfg.service_count, 1,
		      "empty-name entry should be skipped");
	zassert_str_equal(cfg.services[0], "valid");
}

/* ── Error handling tests ───────────────────────────────────────── */

ZTEST(config_parser, test_unsupported_version)
{
	struct ceell_config cfg;
	const char *json =
		"{\"version\":99,\"node_id\":1,\"role\":\"body\","
		"\"name\":\"body-ecu\",\"ip\":\"192.168.1.10\","
		"\"netmask\":\"255.255.255.0\"}";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0, "unsupported version -> defaults");
	zassert_false(cfg.from_flash);
	zassert_equal(cfg.service_count, 0);
}

ZTEST(config_parser, test_invalid_json)
{
	struct ceell_config cfg;
	const char *json = "{not valid json at all!!!";

	setup_flash(json);
	int ret = ceell_config_load(&cfg);

	zassert_equal(ret, 0, "invalid JSON -> defaults");
	zassert_false(cfg.from_flash);
}
