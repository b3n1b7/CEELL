#include "node_identity.h"

#include <string.h>
#include <zephyr/kernel.h>

static struct ceell_config identity;
static bool initialized;

int ceell_identity_init(const struct ceell_config *cfg)
{
	if (initialized) {
		printk("CEELL: identity already initialized\n");
		return -EALREADY;
	}

	memcpy(&identity, cfg, sizeof(identity));
	initialized = true;

	printk("CEELL: Identity set — node_id=%u role=%s name=%s ip=%s\n",
	       identity.node_id, identity.role, identity.name, identity.ip);
	return 0;
}

const struct ceell_config *ceell_identity_get(void)
{
	if (!initialized) {
		return NULL;
	}
	return &identity;
}
