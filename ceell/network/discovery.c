#include "discovery.h"
#include "node_identity.h"
#include "service_registry.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/data/json.h>

#define DISCOVERY_TX_STACK_SIZE 2048
#define DISCOVERY_RX_STACK_SIZE 2048
#define DISCOVERY_TX_PRIORITY   7
#define DISCOVERY_RX_PRIORITY   7
#define ANNOUNCE_BUF_SIZE       512
#define RECV_BUF_SIZE           512

static int sock = -1;
static struct ceell_peer peers[CONFIG_CEELL_DISCOVERY_MAX_PEERS];
static int peer_count;
static struct k_mutex peer_mutex;

K_THREAD_STACK_DEFINE(ceell_disc_tx_stack, DISCOVERY_TX_STACK_SIZE);
K_THREAD_STACK_DEFINE(ceell_disc_rx_stack, DISCOVERY_RX_STACK_SIZE);
static struct k_thread tx_thread_data;
static struct k_thread rx_thread_data;

/* JSON descriptors for announcement encode/decode */
struct discovery_msg {
	int node_id;
	const char *role;
	const char *name;
	const char *ip;
	const char *services;
};

static const struct json_obj_descr discovery_msg_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct discovery_msg, node_id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct discovery_msg, role, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct discovery_msg, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct discovery_msg, ip, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct discovery_msg, services, JSON_TOK_STRING),
};

static void update_peer(const struct discovery_msg *msg)
{
	const struct ceell_config *self = ceell_identity_get();

	/* Ignore our own announcements */
	if (self && (uint32_t)msg->node_id == self->node_id) {
		return;
	}

	k_mutex_lock(&peer_mutex, K_FOREVER);

	/* Update existing peer */
	for (int i = 0; i < peer_count; i++) {
		if (peers[i].node_id == (uint32_t)msg->node_id) {
			peers[i].last_seen = k_uptime_get();
			if (msg->services) {
				strncpy(peers[i].services, msg->services,
					sizeof(peers[i].services) - 1);
				peers[i].services[sizeof(peers[i].services) - 1] = '\0';
			}
			k_mutex_unlock(&peer_mutex);
			return;
		}
	}

	/* Add new peer */
	if (peer_count < CONFIG_CEELL_DISCOVERY_MAX_PEERS) {
		struct ceell_peer *p = &peers[peer_count];

		memset(p, 0, sizeof(*p));
		p->node_id = (uint32_t)msg->node_id;
		if (msg->role) {
			strncpy(p->role, msg->role, sizeof(p->role) - 1);
		}
		if (msg->name) {
			strncpy(p->name, msg->name, sizeof(p->name) - 1);
		}
		if (msg->ip) {
			strncpy(p->ip, msg->ip, sizeof(p->ip) - 1);
		}
		if (msg->services) {
			strncpy(p->services, msg->services,
				sizeof(p->services) - 1);
		}
		p->last_seen = k_uptime_get();
		peer_count++;

		printk("CEELL_DISCOVERED node_id=%u role=%s name=%s ip=%s services=%s\n",
		       p->node_id, p->role, p->name, p->ip, p->services);
	}

	k_mutex_unlock(&peer_mutex);
}

static void tx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static char buf[ANNOUNCE_BUF_SIZE];
	static char svc_csv[128];
	struct sockaddr_in mcast_addr;

	memset(&mcast_addr, 0, sizeof(mcast_addr));
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons(CONFIG_CEELL_DISCOVERY_PORT);
	zsock_inet_pton(AF_INET, CONFIG_CEELL_DISCOVERY_GROUP,
			&mcast_addr.sin_addr);

	while (1) {
		const struct ceell_config *id = ceell_identity_get();

		if (!id) {
			k_sleep(K_MSEC(CONFIG_CEELL_DISCOVERY_INTERVAL_MS));
			continue;
		}

		ceell_service_names_csv(svc_csv, sizeof(svc_csv));

		struct discovery_msg msg = {
			.node_id = (int)id->node_id,
			.role = id->role,
			.name = id->name,
			.ip = id->ip,
			.services = svc_csv,
		};

		int ret = json_obj_encode_buf(discovery_msg_descr,
					      ARRAY_SIZE(discovery_msg_descr),
					      &msg, buf, sizeof(buf));
		if (ret == 0) {
			zsock_sendto(sock, buf, strlen(buf), 0,
				     (struct sockaddr *)&mcast_addr,
				     sizeof(mcast_addr));
		}

		k_sleep(K_MSEC(CONFIG_CEELL_DISCOVERY_INTERVAL_MS));
	}
}

static void rx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static char buf[RECV_BUF_SIZE];

	while (1) {
		struct sockaddr_in src_addr;
		socklen_t addr_len = sizeof(src_addr);

		int len = zsock_recvfrom(sock, buf, sizeof(buf) - 1, 0,
					 (struct sockaddr *)&src_addr,
					 &addr_len);
		if (len <= 0) {
			continue;
		}

		buf[len] = '\0';

		struct discovery_msg msg = { 0 };
		int ret = json_obj_parse(buf, len,
					 discovery_msg_descr,
					 ARRAY_SIZE(discovery_msg_descr),
					 &msg);
		if (ret < 0) {
			continue;
		}

		update_peer(&msg);
	}
}

int ceell_discovery_init(void)
{
	struct sockaddr_in bind_addr;
	struct ip_mreqn mreq;
	int ret;

	k_mutex_init(&peer_mutex);

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printk("CEELL: discovery socket failed (%d)\n", sock);
		return sock;
	}

	/* Allow address reuse */
	int optval = 1;

	zsock_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			 &optval, sizeof(optval));

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(CONFIG_CEELL_DISCOVERY_PORT);
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = zsock_bind(sock, (struct sockaddr *)&bind_addr,
			 sizeof(bind_addr));
	if (ret < 0) {
		printk("CEELL: discovery bind failed (%d)\n", ret);
		return ret;
	}

	/* Join multicast group */
	memset(&mreq, 0, sizeof(mreq));
	zsock_inet_pton(AF_INET, CONFIG_CEELL_DISCOVERY_GROUP,
			&mreq.imr_multiaddr);
	mreq.imr_ifindex = 0;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			       &mreq, sizeof(mreq));
	if (ret < 0) {
		printk("CEELL: multicast join failed (%d) — continuing anyway\n",
		       ret);
		/* Non-fatal: Renode switch may broadcast all packets */
	}

	printk("CEELL: Discovery initialized on port %d, group %s\n",
	       CONFIG_CEELL_DISCOVERY_PORT, CONFIG_CEELL_DISCOVERY_GROUP);
	return 0;
}

void ceell_discovery_start(void)
{
	k_thread_create(&tx_thread_data, ceell_disc_tx_stack,
			K_THREAD_STACK_SIZEOF(ceell_disc_tx_stack),
			tx_thread_fn, NULL, NULL, NULL,
			DISCOVERY_TX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_data, "ceell_disc_tx");

	k_thread_create(&rx_thread_data, ceell_disc_rx_stack,
			K_THREAD_STACK_SIZEOF(ceell_disc_rx_stack),
			rx_thread_fn, NULL, NULL, NULL,
			DISCOVERY_RX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "ceell_disc_rx");

	printk("CEELL: Discovery threads started\n");
}

int ceell_discovery_peer_count(void)
{
	int count;

	k_mutex_lock(&peer_mutex, K_FOREVER);
	count = peer_count;
	k_mutex_unlock(&peer_mutex);
	return count;
}

int ceell_discovery_get_peers(struct ceell_peer *out, int max_peers)
{
	int copy;

	k_mutex_lock(&peer_mutex, K_FOREVER);
	copy = (peer_count < max_peers) ? peer_count : max_peers;
	memcpy(out, peers, copy * sizeof(struct ceell_peer));
	k_mutex_unlock(&peer_mutex);
	return copy;
}

int ceell_discovery_expire_peers(void)
{
	int64_t now = k_uptime_get();
	int expired = 0;

	k_mutex_lock(&peer_mutex, K_FOREVER);

	int i = 0;

	while (i < peer_count) {
		if ((now - peers[i].last_seen) > CONFIG_CEELL_PEER_TIMEOUT_MS) {
			printk("CEELL_PEER_EXPIRED node_id=%u name=%s\n",
			       peers[i].node_id, peers[i].name);
			/* Swap with last entry to compact */
			if (i < peer_count - 1) {
				peers[i] = peers[peer_count - 1];
			}
			peer_count--;
			expired++;
		} else {
			i++;
		}
	}

	k_mutex_unlock(&peer_mutex);

	if (expired > 0) {
		printk("CEELL: expired %d stale peers\n", expired);
	}
	return expired;
}
