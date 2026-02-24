#ifndef CEELL_SERVICE_REGISTRY_H
#define CEELL_SERVICE_REGISTRY_H

#include <zephyr/kernel.h>

struct ceell_msg;

/**
 * Service handler callback. Called when a request arrives for this service.
 *
 * @param payload Request payload string
 * @param rsp_payload Buffer to write response payload into
 * @param rsp_len Size of rsp_payload buffer
 * @return 0 on success, negative errno on failure (sent as status in response)
 */
typedef int (*ceell_service_handler_t)(const char *payload,
				       char *rsp_payload, size_t rsp_len);

/**
 * Initialize the service registry. Must be called once at boot before
 * registering any services.
 */
void ceell_service_init(void);

/**
 * Register a named service with its handler.
 *
 * @param name Service name (max 31 chars, copied internally)
 * @param handler Callback invoked on incoming requests for this service
 * @return 0 on success, -ENOMEM if registry full, -EINVAL if args invalid
 */
int ceell_service_register(const char *name, ceell_service_handler_t handler);

/**
 * Get the number of registered services.
 */
int ceell_service_count(void);

/**
 * Get the name of a registered service by index.
 *
 * @param index 0-based index
 * @return Service name or NULL if index out of range
 */
const char *ceell_service_get_name(int index);

/**
 * Look up a service handler by name.
 *
 * @param name Service name to find
 * @return Handler function or NULL if not found
 */
ceell_service_handler_t ceell_service_get_handler(const char *name);

/**
 * Build a comma-separated string of all registered service names.
 *
 * @param buf Output buffer
 * @param buf_len Size of output buffer
 * @return 0 on success, -ENOMEM if buffer too small
 */
int ceell_service_names_csv(char *buf, size_t buf_len);

#endif /* CEELL_SERVICE_REGISTRY_H */
