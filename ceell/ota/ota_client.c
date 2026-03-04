/* REQ-OTA-001 */ /* REQ-OTA-002 */ /* REQ-OTA-003 */
/* REQ-SAF-004 */

#include "ota_client.h"
#include "ota_state.h"
#include "ota_version.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "osal.h"
#include "osal_json.h"
#include "osal_dfu.h"
#include "osal_http.h"
#include "osal_stream_flash.h"

CEELL_LOG_MODULE_REGISTER(ceell_ota, CEELL_LOG_LEVEL_INF);

/* -- Manifest JSON parsing ---------------------------------------- */

struct ota_manifest {
	const char *version;
	const char *url;
	const char *sha256;
	int size;
};

static const struct json_obj_descr manifest_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct ota_manifest, version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ota_manifest, url, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ota_manifest, sha256, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ota_manifest, size, JSON_TOK_NUMBER),
};

/* -- HTTP receive buffer ------------------------------------------ */

#define HTTP_BUF_SIZE 1024
static uint8_t http_buf[HTTP_BUF_SIZE];

/* -- Stream flash context for writing firmware to slot1 ----------- */

#define FLASH_WRITE_BUF_SIZE 256
static uint8_t flash_write_buf[FLASH_WRITE_BUF_SIZE];
static ceell_stream_flash_ctx_t stream_ctx;
static bool stream_active;

/* -- Manual trigger semaphore ------------------------------------- */

static ceell_sem_t check_trigger;

void ceell_ota_trigger_check(void)
{
	ceell_sem_give(&check_trigger);
}

/* -- HTTP helpers ------------------------------------------------- */

static int connect_to_server(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(CONFIG_CEELL_OTA_SERVER_PORT),
	};
	int sock;
	int ret;

	ret = ceell_inet_pton(AF_INET, CONFIG_CEELL_OTA_SERVER_HOST,
			      &addr.sin_addr);
	if (ret != 1) {
		CEELL_LOG_ERR("Invalid server address: %s",
			      CONFIG_CEELL_OTA_SERVER_HOST);
		return -EINVAL;
	}

	sock = ceell_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		CEELL_LOG_ERR("Socket create failed: %d", errno);
		return -errno;
	}

	ret = ceell_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		CEELL_LOG_ERR("Connect to %s:%d failed: %d",
			      CONFIG_CEELL_OTA_SERVER_HOST,
			      CONFIG_CEELL_OTA_SERVER_PORT, errno);
		ceell_close(sock);
		return -errno;
	}

	return sock;
}

/* -- Manifest fetch ----------------------------------------------- */

static char manifest_buf[512];
static size_t manifest_len;
static bool manifest_truncated;

static void manifest_cb(ceell_http_response_t *rsp,
			ceell_http_final_call_t final_data, void *user_data)
{
	ARG_UNUSED(user_data);

	if (rsp->body_frag_len > 0 && !manifest_truncated) {
		size_t available = sizeof(manifest_buf) - 1 - manifest_len;

		if (rsp->body_frag_len > available) {
			manifest_truncated = true;
			CEELL_LOG_ERR("Manifest too large for buffer (%zu bytes)",
				      manifest_len + rsp->body_frag_len);
			return;
		}
		memcpy(manifest_buf + manifest_len, rsp->body_frag_start,
		       rsp->body_frag_len);
		manifest_len += rsp->body_frag_len;
	}
}

static int fetch_manifest(struct ota_manifest *manifest)
{
	ceell_http_request_t req = { 0 };
	int sock;
	int ret;

	sock = connect_to_server();
	if (sock < 0) {
		return sock;
	}

	manifest_len = 0;
	manifest_truncated = false;

	req.method = CEELL_HTTP_GET;
	req.url = CONFIG_CEELL_OTA_MANIFEST_PATH;
	req.host = CONFIG_CEELL_OTA_SERVER_HOST;
	req.protocol = "HTTP/1.1";
	req.response = manifest_cb;
	req.recv_buf = http_buf;
	req.recv_buf_len = sizeof(http_buf);

	ret = ceell_http_client_req(sock, &req, 10000, NULL);
	ceell_close(sock);

	if (ret < 0) {
		CEELL_LOG_ERR("HTTP manifest request failed: %d", ret);
		return ret;
	}

	if (manifest_truncated) {
		CEELL_LOG_ERR("Manifest truncated — buffer too small");
		return -ENOBUFS;
	}

	manifest_buf[manifest_len] = '\0';

	ret = json_obj_parse(manifest_buf, manifest_len, manifest_descr,
			     ARRAY_SIZE(manifest_descr), manifest);
	if (ret < 0) {
		CEELL_LOG_ERR("Manifest JSON parse failed: %d", ret);
		return -EPROTO;
	}

	return 0;
}

/* -- Firmware download with stream-flash -------------------------- */

static size_t firmware_downloaded;

static void firmware_cb(ceell_http_response_t *rsp,
			ceell_http_final_call_t final_data, void *user_data)
{
	int ret;

	ARG_UNUSED(user_data);

	if (rsp->body_frag_len > 0 && stream_active) {
		ret = ceell_stream_flash_write(&stream_ctx,
					       rsp->body_frag_start,
					       rsp->body_frag_len, false);
		if (ret < 0) {
			CEELL_LOG_ERR("Flash write failed: %d", ret);
			stream_active = false;
			return;
		}
		firmware_downloaded += rsp->body_frag_len;
	}

	if (final_data == CEELL_HTTP_DATA_FINAL && stream_active) {
		ret = ceell_stream_flash_write(&stream_ctx, NULL, 0, true);
		if (ret < 0) {
			CEELL_LOG_ERR("Flash write flush failed: %d", ret);
			stream_active = false;
		}
	}
}

static int download_firmware(const char *url, int expected_size)
{
	ceell_flash_area_t *fa;
	ceell_http_request_t req = { 0 };
	int sock;
	int ret;

	/* Validate URL is an absolute path */
	if (!url || url[0] != '/') {
		CEELL_LOG_ERR("Manifest URL must be an absolute path, got: %s",
			      url ? url : "(null)");
		return -EPROTO;
	}

	ret = ceell_flash_open(CEELL_FIXED_PARTITION_ID(slot1_partition), &fa);
	if (ret < 0) {
		CEELL_LOG_ERR("Failed to open slot1: %d", ret);
		return ret;
	}

	ret = ceell_flash_erase(fa, 0, ceell_flash_area_size(fa));
	if (ret < 0) {
		CEELL_LOG_ERR("Failed to erase slot1: %d", ret);
		ceell_flash_close(fa);
		return ret;
	}

	ret = ceell_stream_flash_init(&stream_ctx,
				      ceell_flash_area_device(fa),
				      flash_write_buf,
				      sizeof(flash_write_buf),
				      ceell_flash_area_offset(fa),
				      ceell_flash_area_size(fa), NULL);
	if (ret < 0) {
		CEELL_LOG_ERR("Stream flash init failed: %d", ret);
		ceell_flash_close(fa);
		return ret;
	}

	stream_active = true;
	firmware_downloaded = 0;

	sock = connect_to_server();
	if (sock < 0) {
		ceell_flash_close(fa);
		return sock;
	}

	req.method = CEELL_HTTP_GET;
	req.url = url;
	req.host = CONFIG_CEELL_OTA_SERVER_HOST;
	req.protocol = "HTTP/1.1";
	req.response = firmware_cb;
	req.recv_buf = http_buf;
	req.recv_buf_len = sizeof(http_buf);

	ret = ceell_http_client_req(sock, &req, 60000, NULL);
	ceell_close(sock);

	/* Always attempt flush if stream was active */
	if (stream_active && ret < 0) {
		ceell_stream_flash_write(&stream_ctx, NULL, 0, true);
	}

	ceell_flash_close(fa);

	if (ret < 0 || !stream_active) {
		CEELL_LOG_ERR("Firmware download failed: %d", ret);
		return ret < 0 ? ret : -EIO;
	}

	if (expected_size > 0 &&
	    firmware_downloaded != (size_t)expected_size) {
		CEELL_LOG_ERR("Size mismatch: got %zu, expected %d",
			      firmware_downloaded, expected_size);
		return -EMSGSIZE;
	}

	CEELL_LOG_INF("Downloaded %zu bytes to slot1", firmware_downloaded);
	return 0;
}

/* -- OTA check cycle ---------------------------------------------- */

static int ota_check_and_update(void)
{
	struct ota_manifest manifest = { 0 };
	struct ceell_ota_ver current, remote;
	char fw_url[128];
	int ret;

	ceell_ota_set_state(CEELL_OTA_CHECKING);
	CEELL_LOG_INF("Checking for firmware update...");

	ret = fetch_manifest(&manifest);
	if (ret < 0) {
		CEELL_LOG_WRN("Manifest fetch failed: %d", ret);
		ceell_ota_set_state(CEELL_OTA_IDLE);
		return ret;
	}

	ceell_ota_ver_current(&current);

	ret = ceell_ota_ver_parse(manifest.version, &remote);
	if (ret < 0) {
		CEELL_LOG_ERR("Bad version in manifest: %s", manifest.version);
		ceell_ota_set_state(CEELL_OTA_IDLE);
		return ret;
	}

	if (ceell_ota_ver_cmp(&remote, &current) <= 0) {
		char ver_str[16];

		ceell_ota_ver_to_str(&current, ver_str, sizeof(ver_str));
		CEELL_LOG_INF("Firmware is current (v%s)", ver_str);
		ceell_ota_set_state(CEELL_OTA_IDLE);
		return 0;
	}

	CEELL_LOG_INF("New firmware available: v%s (size %d)",
		      manifest.version, manifest.size);

	/* Copy URL out of manifest_buf before it could be reused */
	strncpy(fw_url, manifest.url, sizeof(fw_url) - 1);
	fw_url[sizeof(fw_url) - 1] = '\0';

	/* Download */
	ceell_ota_set_state(CEELL_OTA_DOWNLOADING);
	ret = download_firmware(fw_url, manifest.size);
	if (ret < 0) {
		ceell_ota_set_state(CEELL_OTA_ERROR);
		return ret;
	}

	/* Confirm current image before requesting swap */
	ceell_ota_set_state(CEELL_OTA_VALIDATING);
	if (!ceell_boot_is_confirmed()) {
		CEELL_LOG_INF("Confirming current image before swap");
		ret = ceell_boot_confirm();
		if (ret < 0) {
			CEELL_LOG_ERR("Failed to confirm current image: %d", ret);
			ceell_ota_set_state(CEELL_OTA_ERROR);
			return ret;
		}
	}

	/* Mark image for test swap */
	ceell_ota_set_state(CEELL_OTA_MARKING);
	ret = ceell_boot_request_upgrade();
	if (ret < 0) {
		CEELL_LOG_ERR("Failed to mark upgrade: %d", ret);
		ceell_ota_set_state(CEELL_OTA_ERROR);
		return ret;
	}

	CEELL_LOG_INF("Firmware marked for swap — rebooting");
	ceell_ota_set_state(CEELL_OTA_REBOOTING);
	ceell_sleep(CEELL_MSEC(500));
	ceell_sys_reboot();

	/* Should not reach here */
	return 0;
}

/* -- OTA service handler ------------------------------------------ */

int ceell_ota_service_handler(const char *payload, char *rsp_payload,
			      size_t rsp_len)
{
	if (strcmp(payload, "status") == 0) {
		snprintf(rsp_payload, rsp_len, "%s",
			 ceell_ota_state_str(ceell_ota_get_state()));
	} else if (strcmp(payload, "version") == 0) {
		struct ceell_ota_ver ver;

		ceell_ota_ver_current(&ver);
		ceell_ota_ver_to_str(&ver, rsp_payload, rsp_len);
	} else if (strcmp(payload, "check") == 0) {
		ceell_ota_trigger_check();
		snprintf(rsp_payload, rsp_len, "triggered");
	} else {
		snprintf(rsp_payload, rsp_len, "unknown command");
		return -EINVAL;
	}
	return 0;
}

/* -- OTA thread --------------------------------------------------- */

#define OTA_STACK_SIZE 4096
#define OTA_PRIORITY   8

static void ota_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int interval_sec = CONFIG_CEELL_OTA_CHECK_INTERVAL_SEC;
	int fail_count = 0;

	CEELL_LOG_INF("OTA thread started (check every %ds)", interval_sec);

	while (1) {
		/* Wait for timer expiry or manual trigger */
		if (ceell_sem_take(&check_trigger,
				   CEELL_SECONDS(interval_sec)) == 0) {
			CEELL_LOG_INF("OTA check triggered manually");
		}

		if (ota_check_and_update() < 0) {
			fail_count++;
			int backoff = fail_count * interval_sec;

			if (backoff > 3600) {
				backoff = 3600;
			}
			CEELL_LOG_WRN("OTA check failed (%d consecutive), "
				      "backing off %ds", fail_count, backoff);
			ceell_sleep(CEELL_SECONDS(backoff));
		} else {
			fail_count = 0;
		}
	}
}

CEELL_THREAD_STACK_DEFINE(ceell_ota_stack, OTA_STACK_SIZE);
static ceell_thread_t ota_thread;

int ceell_ota_init(void)
{
	int ret;

#ifdef CONFIG_CEELL_OTA_AUTO_CONFIRM
	if (!ceell_boot_is_confirmed()) {
		CEELL_LOG_INF("Auto-confirming current firmware image");
		ret = ceell_boot_confirm();
		if (ret < 0) {
			CEELL_LOG_ERR("Auto-confirm failed: %d", ret);
			return ret;
		}
	}
#endif

	ceell_sem_init(&check_trigger, 0, 1);

	ceell_thread_create(&ota_thread, ceell_ota_stack,
			    CEELL_THREAD_STACK_SIZEOF(ceell_ota_stack),
			    ota_thread_entry, NULL, NULL, NULL,
			    OTA_PRIORITY);
	ceell_thread_name_set(&ota_thread, "ceell_ota");

	CEELL_LOG_INF("OTA initialized");
	return 0;
}
