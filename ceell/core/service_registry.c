/* REQ-SAF-005 */

#include "service_registry.h"

#include <string.h>
#include <errno.h>
#include "osal.h"

#define MAX_SERVICE_NAME_LEN 32

struct service_entry {
	char name[MAX_SERVICE_NAME_LEN];
	ceell_service_handler_t handler;
	uint8_t  priority;    /* 0=CRITICAL, 1=NORMAL, 2=BULK */
	uint16_t deadline_ms; /* 0 = no deadline */
};

static struct service_entry services[CONFIG_CEELL_MAX_SERVICES];
static int svc_count;
static ceell_mutex_t svc_mutex;

void ceell_service_init(void)
{
	ceell_mutex_init(&svc_mutex);
	svc_count = 0;
	memset(services, 0, sizeof(services));
	ceell_printk("CEELL: Service registry initialized (max=%d)\n",
		     CONFIG_CEELL_MAX_SERVICES);
}

int ceell_service_register_sla(const char *name,
			       ceell_service_handler_t handler,
			       uint8_t priority, uint16_t deadline_ms)
{
	if (!name) {
		return -EINVAL;
	}

	ceell_mutex_lock(&svc_mutex);

	if (svc_count >= CONFIG_CEELL_MAX_SERVICES) {
		ceell_mutex_unlock(&svc_mutex);
		ceell_printk("CEELL: Service registry full, cannot register "
			     "'%s'\n", name);
		return -ENOMEM;
	}

	struct service_entry *e = &services[svc_count];

	strncpy(e->name, name, MAX_SERVICE_NAME_LEN - 1);
	e->name[MAX_SERVICE_NAME_LEN - 1] = '\0';
	e->handler = handler;
	e->priority = priority;
	e->deadline_ms = deadline_ms;
	svc_count++;

	ceell_mutex_unlock(&svc_mutex);

	ceell_printk("CEELL_SERVICE_REGISTERED name=%s pri=%u deadline=%u\n",
		     e->name, e->priority, e->deadline_ms);
	return 0;
}

int ceell_service_register(const char *name, ceell_service_handler_t handler)
{
	return ceell_service_register_sla(name, handler, 1, 0);
}

int ceell_service_count(void)
{
	int count;

	ceell_mutex_lock(&svc_mutex);
	count = svc_count;
	ceell_mutex_unlock(&svc_mutex);
	return count;
}

const char *ceell_service_get_name(int index)
{
	const char *name = NULL;

	ceell_mutex_lock(&svc_mutex);
	if (index >= 0 && index < svc_count) {
		name = services[index].name;
	}
	ceell_mutex_unlock(&svc_mutex);
	return name;
}

ceell_service_handler_t ceell_service_get_handler(const char *name)
{
	ceell_service_handler_t handler = NULL;

	if (!name) {
		return NULL;
	}

	ceell_mutex_lock(&svc_mutex);
	for (int i = 0; i < svc_count; i++) {
		if (strcmp(services[i].name, name) == 0) {
			handler = services[i].handler;
			break;
		}
	}
	ceell_mutex_unlock(&svc_mutex);
	return handler;
}

uint8_t ceell_service_get_priority(const char *name)
{
	uint8_t priority = 1; /* NORMAL default */

	if (!name) {
		return priority;
	}

	ceell_mutex_lock(&svc_mutex);
	for (int i = 0; i < svc_count; i++) {
		if (strcmp(services[i].name, name) == 0) {
			priority = services[i].priority;
			break;
		}
	}
	ceell_mutex_unlock(&svc_mutex);
	return priority;
}

uint16_t ceell_service_get_deadline(const char *name)
{
	uint16_t deadline = 0;

	if (!name) {
		return deadline;
	}

	ceell_mutex_lock(&svc_mutex);
	for (int i = 0; i < svc_count; i++) {
		if (strcmp(services[i].name, name) == 0) {
			deadline = services[i].deadline_ms;
			break;
		}
	}
	ceell_mutex_unlock(&svc_mutex);
	return deadline;
}

int ceell_service_names_csv(char *buf, size_t buf_len)
{
	if (!buf || buf_len == 0) {
		return -EINVAL;
	}

	ceell_mutex_lock(&svc_mutex);

	buf[0] = '\0';
	size_t pos = 0;

	for (int i = 0; i < svc_count; i++) {
		size_t name_len = strlen(services[i].name);
		size_t need = name_len + (i > 0 ? 1 : 0); /* comma + name */

		if (pos + need >= buf_len) {
			ceell_mutex_unlock(&svc_mutex);
			return -ENOMEM;
		}

		if (i > 0) {
			buf[pos++] = ',';
		}
		memcpy(buf + pos, services[i].name, name_len);
		pos += name_len;
	}

	buf[pos] = '\0';
	ceell_mutex_unlock(&svc_mutex);
	return 0;
}
