/* REQ-MSG-001 */ /* REQ-MSG-002 */ /* REQ-MSG-003 */
/* REQ-MSG-004 */ /* REQ-MSG-005 */
/* REQ-SAF-004 */ /* REQ-SAF-005 */

#include "messaging.h"
#include "msg_priority.h"
#include "msg_deadline.h"
#include "service_registry.h"
#include "service_discovery.h"
#include "node_identity.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "osal.h"
#include "osal_json.h"

#define MSG_BUF_SIZE 512

/* Thread stack sizes */
#define MSG_DISPATCHER_STACK_SIZE 2048
#define MSG_HANDLER_STACK_SIZE   2048

static int msg_sock = -1;

/* ── Dispatcher thread ─────────────────────────────────────────── */
CEELL_THREAD_STACK_DEFINE(ceell_msg_disp_stack, MSG_DISPATCHER_STACK_SIZE);
static ceell_thread_t msg_disp_thread_data;

/* ── Per-priority handler threads ──────────────────────────────── */
CEELL_THREAD_STACK_DEFINE(ceell_msg_crit_stack, MSG_HANDLER_STACK_SIZE);
static ceell_thread_t msg_crit_thread_data;

CEELL_THREAD_STACK_DEFINE(ceell_msg_norm_stack, MSG_HANDLER_STACK_SIZE);
static ceell_thread_t msg_norm_thread_data;

CEELL_THREAD_STACK_DEFINE(ceell_msg_bulk_stack, MSG_HANDLER_STACK_SIZE);
static ceell_thread_t msg_bulk_thread_data;

/* ── Per-priority message queues ───────────────────────────────── */

/**
 * Internal message passed from dispatcher to per-priority handler.
 * Contains everything needed to process a request and send a response.
 */
struct msg_dispatch_item {
	char payload[128];
	char to_svc[32];
	int  seq;
	int  from_id;
	int  priority;
	int  deadline_ms;
	int64_t send_time_ms;
	struct sockaddr_in src_addr;
};

static ceell_msgq_t crit_msgq;
static ceell_msgq_t norm_msgq;
static ceell_msgq_t bulk_msgq;

/* Message queue backing buffers */
static char crit_msgq_buf[sizeof(struct msg_dispatch_item) *
			   CONFIG_CEELL_MSG_QUEUE_DEPTH];
static char norm_msgq_buf[sizeof(struct msg_dispatch_item) *
			   CONFIG_CEELL_MSG_QUEUE_DEPTH];
static char bulk_msgq_buf[sizeof(struct msg_dispatch_item) *
			   CONFIG_CEELL_MSG_QUEUE_DEPTH];

/* ── Per-priority response slots ───────────────────────────────── */
static ceell_sem_t   rsp_sem[CEELL_MSG_PRIORITY_COUNT];
static struct ceell_msg pending_rsp[CEELL_MSG_PRIORITY_COUNT];
static uint32_t      pending_seq[CEELL_MSG_PRIORITY_COUNT];
static ceell_mutex_t rsp_mutex;

static uint32_t next_seq;

/* ── JSON descriptors for request messages ─────────────────────── */
struct msg_req_json {
	const char *type;
	int seq;
	int from_id;
	const char *to_svc;
	const char *payload;
	int priority;
	int deadline_ms;
	int64_t send_time_ms;
};

static const struct json_obj_descr msg_req_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, seq, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, from_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, to_svc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, payload, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, priority, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, deadline_ms, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_req_json, send_time_ms, JSON_TOK_NUMBER),
};

/* ── JSON descriptors for response messages ────────────────────── */
struct msg_rsp_json {
	const char *type;
	int seq;
	int from_id;
	int status;
	const char *payload;
	int priority;
};

static const struct json_obj_descr msg_rsp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, seq, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, from_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, status, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, payload, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct msg_rsp_json, priority, JSON_TOK_NUMBER),
};

/* ── Helpers ───────────────────────────────────────────────────── */

static ceell_msgq_t *msgq_for_priority(enum ceell_msg_priority pri)
{
	switch (pri) {
	case CEELL_MSG_CRITICAL: return &crit_msgq;
	case CEELL_MSG_NORMAL:   return &norm_msgq;
	case CEELL_MSG_BULK:     return &bulk_msgq;
	default:                 return &norm_msgq;
	}
}

static void send_response(const struct sockaddr_in *dest, int seq,
			   int status, const char *payload,
			   enum ceell_msg_priority priority)
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
		.priority = (int)priority,
	};

	int ret = json_obj_encode_buf(msg_rsp_descr,
				      ARRAY_SIZE(msg_rsp_descr),
				      &rsp, buf, sizeof(buf));
	if (ret == 0) {
		ceell_sendto(msg_sock, buf, strlen(buf), 0,
			     (const struct sockaddr *)dest,
			     sizeof(*dest));
	}
}

/* ── Per-priority handler thread ───────────────────────────────── */

static void handler_thread_fn(void *p1, void *p2, void *p3)
{
	ceell_msgq_t *queue = (ceell_msgq_t *)p1;
	enum ceell_msg_priority pri = (enum ceell_msg_priority)(intptr_t)p2;

	ARG_UNUSED(p3);

	struct msg_dispatch_item item;

	while (1) {
		int ret = ceell_msgq_get(queue, &item, CEELL_FOREVER);
		if (ret != 0) {
			continue;
		}

		/* Check deadline before dispatching */
		if (item.deadline_ms > 0) {
			struct ceell_msg dm = {
				.deadline_ms = (uint16_t)item.deadline_ms,
				.send_time_ms = item.send_time_ms,
			};
			if (ceell_msg_deadline_expired(&dm)) {
				ceell_printk("CEELL_MSG: dropped expired %s "
					     "msg svc=%s seq=%d\n",
					     ceell_msg_priority_name(pri),
					     item.to_svc, item.seq);
				send_response(&item.src_addr, item.seq,
					      -ETIMEDOUT,
					      "deadline expired", pri);
				continue;
			}
		}

		ceell_service_handler_t handler =
			ceell_service_get_handler(item.to_svc);

		if (!handler) {
			ceell_printk("CEELL_MSG: no handler for service "
				     "'%s'\n", item.to_svc);
			send_response(&item.src_addr, item.seq, -ENOENT,
				      "unknown service", pri);
			continue;
		}

		char rsp_payload[128] = "";
		int status = handler(item.payload, rsp_payload,
				     sizeof(rsp_payload));

		ceell_printk("CEELL_MSG_HANDLED svc=%s seq=%d status=%d "
			     "pri=%s\n", item.to_svc, item.seq, status,
			     ceell_msg_priority_name(pri));

		send_response(&item.src_addr, item.seq, status,
			      rsp_payload, pri);
	}
}

/* ── Response handler (called from dispatcher) ─────────────────── */

static void handle_response(const struct msg_rsp_json *rsp)
{
	enum ceell_msg_priority pri = CEELL_MSG_NORMAL;

	if (ceell_msg_priority_valid(rsp->priority)) {
		pri = (enum ceell_msg_priority)rsp->priority;
	}

	ceell_mutex_lock(&rsp_mutex);

	if ((uint32_t)rsp->seq == pending_seq[pri]) {
		pending_rsp[pri].status = rsp->status;
		pending_rsp[pri].priority = pri;
		if (rsp->payload) {
			strncpy(pending_rsp[pri].payload, rsp->payload,
				sizeof(pending_rsp[pri].payload) - 1);
			pending_rsp[pri].payload[
				sizeof(pending_rsp[pri].payload) - 1] = '\0';
		} else {
			pending_rsp[pri].payload[0] = '\0';
		}
		ceell_sem_give(&rsp_sem[pri]);
	}

	ceell_mutex_unlock(&rsp_mutex);
}

/* ── Dispatcher thread: reads socket, routes to per-priority queues ── */

static void msg_dispatcher_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static char buf[MSG_BUF_SIZE];

	while (1) {
		struct sockaddr_in src_addr;
		socklen_t addr_len = sizeof(src_addr);

		int len = ceell_recvfrom(msg_sock, buf, sizeof(buf) - 1, 0,
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
			/* Determine priority — default to NORMAL */
			enum ceell_msg_priority pri = CEELL_MSG_NORMAL;

			if (ceell_msg_priority_valid(req.priority)) {
				pri = (enum ceell_msg_priority)req.priority;
			}

			/* Build dispatch item */
			struct msg_dispatch_item item;

			memset(&item, 0, sizeof(item));
			if (req.payload) {
				strncpy(item.payload, req.payload,
					sizeof(item.payload) - 1);
			}
			if (req.to_svc) {
				strncpy(item.to_svc, req.to_svc,
					sizeof(item.to_svc) - 1);
			}
			item.seq = req.seq;
			item.from_id = req.from_id;
			item.priority = (int)pri;
			item.deadline_ms = req.deadline_ms;
			item.send_time_ms = req.send_time_ms;
			memcpy(&item.src_addr, &src_addr, sizeof(src_addr));

			ceell_msgq_t *queue = msgq_for_priority(pri);

			ret = ceell_msgq_put(queue, &item, CEELL_NO_WAIT);
			if (ret != 0) {
				ceell_printk("CEELL_MSG: %s queue full, "
					     "dropped svc=%s seq=%d\n",
					     ceell_msg_priority_name(pri),
					     item.to_svc, item.seq);
				send_response(&src_addr, req.seq,
					      -ENOMEM,
					      "queue full", pri);
			}
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

/* ── Public API ────────────────────────────────────────────────── */

int ceell_messaging_init(void)
{
	struct sockaddr_in bind_addr;
	int ret;

	/* Initialize per-priority response semaphores */
	for (int i = 0; i < CEELL_MSG_PRIORITY_COUNT; i++) {
		ceell_sem_init(&rsp_sem[i], 0, 1);
		pending_seq[i] = 0;
		memset(&pending_rsp[i], 0, sizeof(pending_rsp[i]));
	}

	ceell_mutex_init(&rsp_mutex);
	next_seq = 1;

	/* Initialize per-priority message queues */
	ceell_msgq_init(&crit_msgq, crit_msgq_buf,
			sizeof(struct msg_dispatch_item),
			CONFIG_CEELL_MSG_QUEUE_DEPTH);
	ceell_msgq_init(&norm_msgq, norm_msgq_buf,
			sizeof(struct msg_dispatch_item),
			CONFIG_CEELL_MSG_QUEUE_DEPTH);
	ceell_msgq_init(&bulk_msgq, bulk_msgq_buf,
			sizeof(struct msg_dispatch_item),
			CONFIG_CEELL_MSG_QUEUE_DEPTH);

	/* Open socket */
	msg_sock = ceell_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (msg_sock < 0) {
		ceell_printk("CEELL: messaging socket failed (%d)\n",
			     msg_sock);
		return msg_sock;
	}

	int optval = 1;

	ceell_setsockopt(msg_sock, SOL_SOCKET, SO_REUSEADDR,
			 &optval, sizeof(optval));

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(CONFIG_CEELL_MSG_PORT);
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = ceell_bind(msg_sock, (struct sockaddr *)&bind_addr,
			 sizeof(bind_addr));
	if (ret < 0) {
		ceell_printk("CEELL: messaging bind failed (%d)\n", ret);
		return ret;
	}

	/* Start dispatcher thread */
	ceell_thread_create(&msg_disp_thread_data, ceell_msg_disp_stack,
			    CEELL_THREAD_STACK_SIZEOF(ceell_msg_disp_stack),
			    msg_dispatcher_fn, NULL, NULL, NULL,
			    CONFIG_CEELL_MSG_DISPATCHER_PRIORITY);
	ceell_thread_name_set(&msg_disp_thread_data, "ceell_msg_disp");

	/* Start per-priority handler threads */
	ceell_thread_create(&msg_crit_thread_data, ceell_msg_crit_stack,
			    CEELL_THREAD_STACK_SIZEOF(ceell_msg_crit_stack),
			    handler_thread_fn,
			    &crit_msgq,
			    (void *)(intptr_t)CEELL_MSG_CRITICAL,
			    NULL,
			    CONFIG_CEELL_MSG_CRITICAL_PRIORITY);
	ceell_thread_name_set(&msg_crit_thread_data, "ceell_msg_crit");

	ceell_thread_create(&msg_norm_thread_data, ceell_msg_norm_stack,
			    CEELL_THREAD_STACK_SIZEOF(ceell_msg_norm_stack),
			    handler_thread_fn,
			    &norm_msgq,
			    (void *)(intptr_t)CEELL_MSG_NORMAL,
			    NULL,
			    CONFIG_CEELL_MSG_NORMAL_PRIORITY);
	ceell_thread_name_set(&msg_norm_thread_data, "ceell_msg_norm");

	ceell_thread_create(&msg_bulk_thread_data, ceell_msg_bulk_stack,
			    CEELL_THREAD_STACK_SIZEOF(ceell_msg_bulk_stack),
			    handler_thread_fn,
			    &bulk_msgq,
			    (void *)(intptr_t)CEELL_MSG_BULK,
			    NULL,
			    CONFIG_CEELL_MSG_BULK_PRIORITY);
	ceell_thread_name_set(&msg_bulk_thread_data, "ceell_msg_bulk");

	ceell_printk("CEELL: Messaging initialized on port %d "
		     "(dispatcher=%d, crit=%d, norm=%d, bulk=%d)\n",
		     CONFIG_CEELL_MSG_PORT,
		     CONFIG_CEELL_MSG_DISPATCHER_PRIORITY,
		     CONFIG_CEELL_MSG_CRITICAL_PRIORITY,
		     CONFIG_CEELL_MSG_NORMAL_PRIORITY,
		     CONFIG_CEELL_MSG_BULK_PRIORITY);
	return 0;
}

int ceell_msg_send_pri(const char *service, const char *payload,
		       struct ceell_msg *rsp, ceell_timeout_t timeout,
		       enum ceell_msg_priority priority,
		       uint16_t deadline_ms)
{
	static char buf[MSG_BUF_SIZE];
	char peer_ip[16];
	int ret;

	if (!service || !rsp) {
		return -EINVAL;
	}

	if (!ceell_msg_priority_valid((int)priority)) {
		priority = CEELL_MSG_NORMAL;
	}

	/* Resolve service -> peer IP */
	ret = ceell_service_find_peer(service, peer_ip);
	if (ret < 0) {
		ceell_printk("CEELL_MSG: service '%s' not found on any "
			     "peer\n", service);
		return -ENOENT;
	}

	const struct ceell_config *id = ceell_identity_get();

	if (!id) {
		return -ENODEV;
	}

	/* Claim the response slot for this priority */
	ceell_mutex_lock(&rsp_mutex);
	uint32_t seq = next_seq++;

	pending_seq[priority] = seq;
	memset(&pending_rsp[priority], 0, sizeof(pending_rsp[priority]));
	ceell_sem_reset(&rsp_sem[priority]);
	ceell_mutex_unlock(&rsp_mutex);

	/* Record send time */
	int64_t send_time = ceell_uptime_get();

	/* Build request JSON */
	struct msg_req_json req = {
		.type = "req",
		.seq = (int)seq,
		.from_id = (int)id->node_id,
		.to_svc = service,
		.payload = payload ? payload : "",
		.priority = (int)priority,
		.deadline_ms = (int)deadline_ms,
		.send_time_ms = send_time,
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
	ceell_inet_pton(AF_INET, peer_ip, &dest.sin_addr);

	ret = ceell_sendto(msg_sock, buf, strlen(buf), 0,
			   (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		return ret;
	}

	ceell_printk("CEELL_MSG_SENT svc=%s peer=%s seq=%u pri=%s "
		     "deadline=%u\n", service, peer_ip, seq,
		     ceell_msg_priority_name(priority), deadline_ms);

	/* Wait for response */
	ret = ceell_sem_take(&rsp_sem[priority], timeout);
	if (ret < 0) {
		ceell_printk("CEELL_MSG_TIMEOUT svc=%s seq=%u pri=%s\n",
			     service, seq,
			     ceell_msg_priority_name(priority));
		return -ETIMEDOUT;
	}

	/* Copy response out */
	ceell_mutex_lock(&rsp_mutex);
	*rsp = pending_rsp[priority];
	rsp->priority = priority;
	rsp->deadline_ms = deadline_ms;
	rsp->send_time_ms = send_time;
	ceell_mutex_unlock(&rsp_mutex);

	ceell_printk("CEELL_MSG_RSP svc=%s seq=%u status=%d pri=%s "
		     "payload=%s\n", service, seq, rsp->status,
		     ceell_msg_priority_name(priority), rsp->payload);
	return 0;
}

int ceell_msg_send(const char *service, const char *payload,
		   struct ceell_msg *rsp, ceell_timeout_t timeout)
{
	return ceell_msg_send_pri(service, payload, rsp, timeout,
				  CEELL_MSG_NORMAL, 0);
}
