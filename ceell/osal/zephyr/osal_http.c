/*
 * CEELL OSAL — Zephyr HTTP Client Implementation
 */

#include "../osal_http.h"
#include <zephyr/net/http/client.h>

int ceell_http_client_req(int sock, ceell_http_request_t *req,
			  int timeout, void *user_data)
{
	return http_client_req(sock, req, timeout, user_data);
}
