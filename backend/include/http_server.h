#ifndef SDM_HTTP_SERVER_H
#define SDM_HTTP_SERVER_H

#include <microhttpd.h>

/* Starts the HTTP server on the given port. Returns a daemon handle that
 * must be passed to http_server_stop() during shutdown, or NULL on failure. */
struct MHD_Daemon *http_server_start(int port);
void http_server_stop(struct MHD_Daemon *daemon);

#endif /* SDM_HTTP_SERVER_H */
