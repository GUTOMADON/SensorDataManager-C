#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handlers.h"

/* Per-connection state used to accumulate the request body across the
 * multiple invocations libmicrohttpd makes while data is still arriving. */
typedef struct {
    char *body;
    size_t body_len;
} RequestContext;

static const char *CORS_ORIGIN_HEADER = "Access-Control-Allow-Origin";
static const char *CORS_METHODS_HEADER = "Access-Control-Allow-Methods";
static const char *CORS_HEADERS_HEADER = "Access-Control-Allow-Headers";

static void add_cors_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, CORS_ORIGIN_HEADER, "*");
    MHD_add_response_header(response, CORS_METHODS_HEADER, "GET, POST, OPTIONS");
    MHD_add_response_header(response, CORS_HEADERS_HEADER, "Content-Type");
}

static int send_json_response(struct MHD_Connection *connection, int status_code, char *body) {
    size_t len = (body != NULL) ? strlen(body) : 0;
    struct MHD_Response *response = MHD_create_response_from_buffer(
        len, body, MHD_RESPMEM_MUST_FREE);

    MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
    add_cors_headers(response);

    int ret = MHD_queue_response(connection, (unsigned int) status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_no_content(struct MHD_Connection *connection, int status_code) {
    struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
    add_cors_headers(response);
    int ret = MHD_queue_response(connection, (unsigned int) status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static ApiResponse route_request(const char *method, const char *url,
                                  struct MHD_Connection *connection, const char *body) {
    if (strcmp(method, "GET") == 0 && strcmp(url, "/api/health") == 0) {
        return handle_health();
    }

    if (strcmp(method, "GET") == 0 && strcmp(url, "/api/equipment") == 0) {
        return handle_list_equipment();
    }

    if (strcmp(method, "GET") == 0 && strcmp(url, "/api/readings") == 0) {
        ReadingQueryParams params;
        params.equipment_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "equipment_id");
        params.sensor_type = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "sensor_type");
        params.from_time = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "from");
        params.to_time = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "to");
        params.limit = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "limit");
        return handle_list_readings(&params);
    }

    if (strcmp(method, "POST") == 0 && strcmp(url, "/api/readings") == 0) {
        return handle_create_reading(body);
    }

    if (strcmp(method, "GET") == 0 && strcmp(url, "/api/stats") == 0) {
        StatsQueryParams params;
        params.equipment_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "equipment_id");
        params.sensor_type = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "sensor_type");
        return handle_get_stats(&params);
    }

    return handle_not_found();
}

/* MHD_AccessHandlerCallback. The return type is plain int (rather than the
 * newer enum MHD_Result) so this file compiles against both older and newer
 * releases of libmicrohttpd; MHD_YES/MHD_NO are valid int values either way. */
static int request_handler(void *cls, struct MHD_Connection *connection, const char *url,
                            const char *method, const char *version,
                            const char *upload_data, size_t *upload_data_size,
                            void **con_cls) {
    (void) cls;
    (void) version;

    if (*con_cls == NULL) {
        RequestContext *ctx = calloc(1, sizeof(RequestContext));
        if (ctx == NULL) {
            return MHD_NO;
        }
        *con_cls = ctx;
        return MHD_YES;
    }

    RequestContext *ctx = (RequestContext *) *con_cls;

    if (*upload_data_size != 0) {
        char *grown = realloc(ctx->body, ctx->body_len + *upload_data_size + 1);
        if (grown == NULL) {
            return MHD_NO;
        }
        ctx->body = grown;
        memcpy(ctx->body + ctx->body_len, upload_data, *upload_data_size);
        ctx->body_len += *upload_data_size;
        ctx->body[ctx->body_len] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        return send_no_content(connection, 204);
    }

    ApiResponse resp = route_request(method, url, connection, ctx->body);
    return send_json_response(connection, resp.status_code, resp.body);
}

static void request_completed(void *cls, struct MHD_Connection *connection,
                               void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void) cls;
    (void) connection;
    (void) toe;

    RequestContext *ctx = (RequestContext *) *con_cls;
    if (ctx != NULL) {
        free(ctx->body);
        free(ctx);
        *con_cls = NULL;
    }
}

struct MHD_Daemon *http_server_start(int port) {
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD,
        (unsigned short) port,
        NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 30,
        MHD_OPTION_END);

    return daemon;
}

void http_server_stop(struct MHD_Daemon *daemon) {
    if (daemon != NULL) {
        MHD_stop_daemon(daemon);
    }
}
