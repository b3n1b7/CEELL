#include "service_registry.h"

#include <string.h>
#include <zephyr/kernel.h>

#define MAX_SERVICE_NAME_LEN 32

struct service_entry {
	char name[MAX_SERVICE_NAME_LEN];
	ceell_service_handler_t handler;
};

static struct service_entry services[CONFIG_CEELL_MAX_SERVICES];
static int svc_count;
static struct k_mutex svc_mutex;

void ceell_service_init(void)
{
	k_mutex_init(&svc_mutex);
	svc_count = 0;
	memset(services, 0, sizeof(services));
	printk("CEELL: Service registry initialized (max=%d)\n",
	       CONFIG_CEELL_MAX_SERVICES);
}

int ceell_service_register(const char *name, ceell_service_handler_t handler)
{
	if (!name) {
		return -EINVAL;
	}

	k_mutex_lock(&svc_mutex, K_FOREVER);

	if (svc_count >= CONFIG_CEELL_MAX_SERVICES) {
		k_mutex_unlock(&svc_mutex);
		printk("CEELL: Service registry full, cannot register '%s'\n",
		       name);
		return -ENOMEM;
	}

	struct service_entry *e = &services[svc_count];

	strncpy(e->name, name, MAX_SERVICE_NAME_LEN - 1);
	e->name[MAX_SERVICE_NAME_LEN - 1] = '\0';
	e->handler = handler;
	svc_count++;

	k_mutex_unlock(&svc_mutex);

	printk("CEELL_SERVICE_REGISTERED name=%s\n", e->name);
	return 0;
}

int ceell_service_count(void)
{
	int count;

	k_mutex_lock(&svc_mutex, K_FOREVER);
	count = svc_count;
	k_mutex_unlock(&svc_mutex);
	return count;
}

const char *ceell_service_get_name(int index)
{
	const char *name = NULL;

	k_mutex_lock(&svc_mutex, K_FOREVER);
	if (index >= 0 && index < svc_count) {
		name = services[index].name;
	}
	k_mutex_unlock(&svc_mutex);
	return name;
}

ceell_service_handler_t ceell_service_get_handler(const char *name)
{
	ceell_service_handler_t handler = NULL;

	if (!name) {
		return NULL;
	}

	k_mutex_lock(&svc_mutex, K_FOREVER);
	for (int i = 0; i < svc_count; i++) {
		if (strcmp(services[i].name, name) == 0) {
			handler = services[i].handler;
			break;
		}
	}
	k_mutex_unlock(&svc_mutex);
	return handler;
}

int ceell_service_names_csv(char *buf, size_t buf_len)
{
	if (!buf || buf_len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&svc_mutex, K_FOREVER);

	buf[0] = '\0';
	size_t pos = 0;

	for (int i = 0; i < svc_count; i++) {
		size_t name_len = strlen(services[i].name);
		size_t need = name_len + (i > 0 ? 1 : 0); /* comma + name */

		if (pos + need >= buf_len) {
			k_mutex_unlock(&svc_mutex);
			return -ENOMEM;
		}

		if (i > 0) {
			buf[pos++] = ',';
		}
		memcpy(buf + pos, services[i].name, name_len);
		pos += name_len;
	}

	buf[pos] = '\0';
	k_mutex_unlock(&svc_mutex);
	return 0;
}
