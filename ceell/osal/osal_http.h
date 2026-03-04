/*
 * CEELL OSAL — HTTP Client
 *
 * Wraps HTTP client operations for OTA manifest/firmware fetching.
 * On Zephyr, wraps <zephyr/net/http/client.h>.
 */

#ifndef CEELL_OSAL_HTTP_H
#define CEELL_OSAL_HTTP_H

#include <stddef.h>
#include <stdint.h>

#if defined(__ZEPHYR__)
#include <zephyr/net/http/client.h>

/* Re-export Zephyr HTTP types for use by OTA client */
typedef struct http_request ceell_http_request_t;
typedef struct http_response ceell_http_response_t;
typedef enum http_final_call ceell_http_final_call_t;

#define CEELL_HTTP_DATA_FINAL HTTP_DATA_FINAL
#define CEELL_HTTP_GET        HTTP_GET

typedef void (*ceell_http_response_cb_t)(struct http_response *rsp,
					 enum http_final_call final_data,
					 void *user_data);

#endif /* __ZEPHYR__ */

/**
 * Perform an HTTP request on an already-connected socket.
 *
 * @param sock    Connected TCP socket
 * @param req     HTTP request descriptor
 * @param timeout Timeout in milliseconds
 * @param user_data Passed to response callback
 * @return 0 on success, negative errno on failure
 */
int ceell_http_client_req(int sock, ceell_http_request_t *req,
			  int timeout, void *user_data);

#endif /* CEELL_OSAL_HTTP_H */
