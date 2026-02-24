#include "messaging.h"
#include "service_registry.h"
#include "service_discovery.h"
#include "node_identity.h"

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/data/json.h>

#define MSG_RX_STACK_SIZE 2048
#define MSG_RX_PRIORITY   7
#define MSG_BUF_SIZE      512

static int msg_sock = -1;

K_THREAD_STACK_DEFINE(ceell_msg_rx_stack, MSG_RX_STACK_SIZE);
static struct k_thread msg_rx_thread_data;

/* Single pending response slot */
static struct k_sem rsp_sem;
static struct ceell_msg pending_rsp;
static uint32_t pending_seq;
static struct k_mutex rsp_mutex;

static uint32_t next_seq;

/* --- JSON descriptors for request messages --- */
struct msg_req_json {
	const char *type;
	int seq;
	int from_id;
	const char *to_svc;
	const char *payload;
};

static const struct json_obj_descr msg_req_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, seq, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, from_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, to_svc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, payload, JSON_TOK_STRING),
};

/* --- JSON descriptors for response messages --- */
struct msg_rsp_json {
	const char *type;
	int seq;
	int from_id;
	int status;
	const char *payload;
};

static const struct json_obj_descr msg_rsp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, seq, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, from_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, status, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, payload, JSON_TOK_STRING),
};

static void send_response(const struct sockaddr_in *dest, int seq,
			   int status, const char *payload)
{
	static char buf[MSG_BUF_SIZE];
	const struct ceell_config *id = ceell_identity_get();

	if (!id) {
		return;
	}

	struct msg_rsp_json rsp = {
		.type = "rsp",
		.seq = seq,
		.from_id = (int)id->node_id,
		.status = status,
		.payload = payload ? payload : "",
	};

	int ret = json_obj_encode_buf(msg_rsp_descr,
				      ARRAY_SIZE(msg_rsp_descr),
				      &rsp, buf, sizeof(buf));
	if (ret == 0) {
		zsock_sendto(msg_sock, buf, strlen(buf), 0,
			     (const struct sockaddr *)dest,
			     sizeof(*dest));
	}
}

static void handle_request(const struct msg_req_json *req,
			   const struct sockaddr_in *src)
{
	if (!req->to_svc) {
		return;
	}

	ceell_service_handler_t handler = ceell_service_get_handler(req->to_svc);

	if (!handler) {
		printk("CEELL_MSG: no handler for service '%s'\n", req->to_svc);
		send_response(src, req->seq, -ENOENT, "unknown service");
		return;
	}

	char rsp_payload[128] = "";
	int status = handler(req->payload ? req->payload : "",
			     rsp_payload, sizeof(rsp_payload));

	printk("CEELL_MSG_HANDLED svc=%s seq=%d status=%d\n",
	       req->to_svc, req->seq, status);

	send_response(src, req->seq, status, rsp_payload);
}

static void handle_response(const struct msg_rsp_json *rsp)
{
	k_mutex_lock(&rsp_mutex, K_FOREVER);

	if ((uint32_t)rsp->seq == pending_seq) {
		pending_rsp.status = rsp->status;
		if (rsp->payload) {
			strncpy(pending_rsp.payload, rsp->payload,
				sizeof(pending_rsp.payload) - 1);
			pending_rsp.payload[sizeof(pending_rsp.payload) - 1] = '\0';
		} else {
			pending_rsp.payload[0] = '\0';
		}
		k_sem_give(&rsp_sem);
	}

	k_mutex_unlock(&rsp_mutex);
}

static void msg_rx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static char buf[MSG_BUF_SIZE];

	while (1) {
		struct sockaddr_in src_addr;
		socklen_t addr_len = sizeof(src_addr);

		int len = zsock_recvfrom(msg_sock, buf, sizeof(buf) - 1, 0,
					 (struct sockaddr *)&src_addr,
					 &addr_len);
		if (len <= 0) {
			continue;
		}

		buf[len] = '\0';

		/* Try parsing as request first */
		struct msg_req_json req = { 0 };
		int ret = json_obj_parse(buf, len, msg_req_descr,
					 ARRAY_SIZE(msg_req_descr), &req);

		if (ret >= 0 && req.type && strcmp(req.type, "req") == 0) {
			handle_request(&req, &src_addr);
			continue;
		}

		/* Try parsing as response */
		struct msg_rsp_json rsp = { 0 };

		ret = json_obj_parse(buf, len, msg_rsp_descr,
				     ARRAY_SIZE(msg_rsp_descr), &rsp);

		if (ret >= 0 && rsp.type && strcmp(rsp.type, "rsp") == 0) {
			handle_response(&rsp);
		}
	}
}

int ceell_messaging_init(void)
{
	struct sockaddr_in bind_addr;
	int ret;

	k_sem_init(&rsp_sem, 0, 1);
	k_mutex_init(&rsp_mutex);
	next_seq = 1;

	msg_sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (msg_sock < 0) {
		printk("CEELL: messaging socket failed (%d)\n", msg_sock);
		return msg_sock;
	}

	int optval = 1;

	zsock_setsockopt(msg_sock, SOL_SOCKET, SO_REUSEADDR,
			 &optval, sizeof(optval));

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(CONFIG_CEELL_MSG_PORT);
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = zsock_bind(msg_sock, (struct sockaddr *)&bind_addr,
			 sizeof(bind_addr));
	if (ret < 0) {
		printk("CEELL: messaging bind failed (%d)\n", ret);
		return ret;
	}

	k_thread_create(&msg_rx_thread_data, ceell_msg_rx_stack,
			K_THREAD_STACK_SIZEOF(ceell_msg_rx_stack),
			msg_rx_thread_fn, NULL, NULL, NULL,
			MSG_RX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&msg_rx_thread_data, "ceell_msg_rx");

	printk("CEELL: Messaging initialized on port %d\n",
	       CONFIG_CEELL_MSG_PORT);
	return 0;
}

int ceell_msg_send(const char *service, const char *payload,
		   struct ceell_msg *rsp, k_timeout_t timeout)
{
	static char buf[MSG_BUF_SIZE];
	char peer_ip[16];
	int ret;

	if (!service || !rsp) {
		return -EINVAL;
	}

	/* Resolve service → peer IP */
	ret = ceell_service_find_peer(service, peer_ip);
	if (ret < 0) {
		printk("CEELL_MSG: service '%s' not found on any peer\n",
		       service);
		return -ENOENT;
	}

	const struct ceell_config *id = ceell_identity_get();

	if (!id) {
		return -ENODEV;
	}

	/* Claim the response slot */
	k_mutex_lock(&rsp_mutex, K_FOREVER);
	uint32_t seq = next_seq++;

	pending_seq = seq;
	memset(&pending_rsp, 0, sizeof(pending_rsp));
	k_sem_reset(&rsp_sem);
	k_mutex_unlock(&rsp_mutex);

	/* Build request JSON */
	struct msg_req_json req = {
		.type = "req",
		.seq = (int)seq,
		.from_id = (int)id->node_id,
		.to_svc = service,
		.payload = payload ? payload : "",
	};

	ret = json_obj_encode_buf(msg_req_descr, ARRAY_SIZE(msg_req_descr),
				  &req, buf, sizeof(buf));
	if (ret != 0) {
		return -ENOMEM;
	}

	/* Send to peer */
	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(CONFIG_CEELL_MSG_PORT);
	zsock_inet_pton(AF_INET, peer_ip, &dest.sin_addr);

	ret = zsock_sendto(msg_sock, buf, strlen(buf), 0,
			   (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		return ret;
	}

	printk("CEELL_MSG_SENT svc=%s peer=%s seq=%u\n", service, peer_ip, seq);

	/* Wait for response */
	ret = k_sem_take(&rsp_sem, timeout);
	if (ret < 0) {
		printk("CEELL_MSG_TIMEOUT svc=%s seq=%u\n", service, seq);
		return -ETIMEDOUT;
	}

	/* Copy response out */
	k_mutex_lock(&rsp_mutex, K_FOREVER);
	*rsp = pending_rsp;
	k_mutex_unlock(&rsp_mutex);

	printk("CEELL_MSG_RSP svc=%s seq=%u status=%d payload=%s\n",
	       service, seq, rsp->status, rsp->payload);
	return 0;
}
