#include "net_setup.h"

#include "osal_log.h"
#include "osal_net.h"

int ceell_net_setup(const struct ceell_config *cfg)
{
	int ret;

	if (!cfg || cfg->ip[0] == '\0') {
		ceell_printk("CEELL: net_setup: no IP configured\n");
		return -EINVAL;
	}

	ret = ceell_net_if_setup(cfg->ip, cfg->netmask);
	if (ret < 0) {
		ceell_printk("CEELL: net_setup: failed (%d)\n", ret);
		return ret;
	}

	ceell_printk("CEELL: net_setup: IP %s netmask %s\n",
		     cfg->ip, cfg->netmask);
	return 0;
}
