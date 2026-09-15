#ifndef SDM_HANDLERS_H
#define SDM_HANDLERS_H

/* Request handlers: one function per API operation. Each handler opens its
 * own short-lived PostgreSQL connection, performs the query, and returns a
 * ready-to-send JSON body plus HTTP status code. Keeping handlers free of
 * any knowledge of libmicrohttpd keeps the HTTP transport layer swappable. */

typedef struct {
    int status_code;
    char *body; /* heap-allocated JSON string; caller must free() it */
} ApiResponse;

typedef struct {
    const char *equipment_id; /* raw query string values, NULL if absent */
    const char *sensor_type;
    const char *from_time;
    const char *to_time;
    const char *limit;
} ReadingQueryParams;

typedef struct {
    const char *equipment_id;
    const char *sensor_type;
} StatsQueryParams;

void handlers_init(const char *conninfo);

ApiResponse handle_health(void);
ApiResponse handle_list_equipment(void);
ApiResponse handle_list_readings(const ReadingQueryParams *params);
ApiResponse handle_create_reading(const char *json_body);
ApiResponse handle_get_stats(const StatsQueryParams *params);
ApiResponse handle_not_found(void);

#endif /* SDM_HANDLERS_H */
