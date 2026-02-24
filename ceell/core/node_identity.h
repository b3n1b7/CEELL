#ifndef CEELL_NODE_IDENTITY_H
#define CEELL_NODE_IDENTITY_H

#include "config_parser.h"

/**
 * Initialize node identity from config. Must be called once at boot.
 *
 * @param cfg Parsed configuration
 * @return 0 on success
 */
int ceell_identity_init(const struct ceell_config *cfg);

/**
 * Get read-only pointer to node identity. Valid after ceell_identity_init().
 */
const struct ceell_config *ceell_identity_get(void);

#endif /* CEELL_NODE_IDENTITY_H */
