#ifndef CEELL_SERVICE_DISCOVERY_H
#define CEELL_SERVICE_DISCOVERY_H

/**
 * Find a peer that offers the given service name.
 * Scans the discovery peer table for a peer whose services CSV contains
 * the requested service.
 *
 * @param service_name Service to look up (e.g. "lighting")
 * @param ip_out Buffer to write peer IP (at least 16 bytes)
 * @return 0 on success, -ENOENT if no peer offers this service
 */
int ceell_service_find_peer(const char *service_name, char *ip_out);

#endif /* CEELL_SERVICE_DISCOVERY_H */
