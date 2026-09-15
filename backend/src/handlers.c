#include "handlers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <json-c/json.h>

#include "db.h"
#include "json_util.h"

static char g_conninfo[512];

void handlers_init(const char *conninfo) {
    snprintf(g_conninfo, sizeof(g_conninfo), "%s", conninfo);
}

static ApiResponse make_response(int status_code, struct json_object *body_obj) {
    ApiResponse resp;
    resp.status_code = status_code;
    const char *json_str = json_object_to_json_string_ext(body_obj, JSON_C_TO_STRING_PRETTY);
    resp.body = strdup(json_str);
    json_object_put(body_obj);
    return resp;
}

static ApiResponse make_error(int status_code, const char *message) {
    return make_response(status_code, json_error(message));
}

static int is_valid_sensor_type(const char *sensor_type) {
    return sensor_type != NULL &&
           (strcmp(sensor_type, "temperature") == 0 ||
            strcmp(sensor_type, "vibration") == 0 ||
            strcmp(sensor_type, "rotational_speed") == 0);
}

static const char *default_unit_for(const char *sensor_type) {
    if (strcmp(sensor_type, "temperature") == 0) {
        return "C";
    }
    if (strcmp(sensor_type, "vibration") == 0) {
        return "mm/s";
    }
    if (strcmp(sensor_type, "rotational_speed") == 0) {
        return "rpm";
    }
    return "";
}

/* Parses a decimal integer strictly: the whole string must be consumed and
 * be non-empty. Returns 0 on success, -1 if the string is not a valid
 * integer. */
static int parse_int_strict(const char *str, int *out) {
    if (str == NULL || str[0] == '\0') {
        return -1;
    }
    char *endptr = NULL;
    errno = 0;
    long value = strtol(str, &endptr, 10);
    if (errno != 0 || endptr == str || *endptr != '\0') {
        return -1;
    }
    *out = (int) value;
    return 0;
}

ApiResponse handle_health(void) {
    char errbuf[256];
    PGconn *conn = db_connect(g_conninfo, errbuf, sizeof(errbuf));

    struct json_object *obj = json_object_new_object();
    if (conn != NULL) {
        json_object_object_add(obj, "status", json_object_new_string("ok"));
        json_object_object_add(obj, "database", json_object_new_string("connected"));
        db_disconnect(conn);
        return make_response(200, obj);
    }

    json_object_object_add(obj, "status", json_object_new_string("degraded"));
    json_object_object_add(obj, "database", json_object_new_string("unreachable"));
    return make_response(503, obj);
}

ApiResponse handle_list_equipment(void) {
    char errbuf[256];
    PGconn *conn = db_connect(g_conninfo, errbuf, sizeof(errbuf));
    if (conn == NULL) {
        return make_error(500, "Failed to connect to the database");
    }

    EquipmentList list = {0};
    int rc = db_list_equipment(conn, &list, errbuf, sizeof(errbuf));
    db_disconnect(conn);

    if (rc != 0) {
        return make_error(500, errbuf);
    }

    struct json_object *body = json_equipment_array(&list);
    equipment_list_free(&list);
    return make_response(200, body);
}

ApiResponse handle_list_readings(const ReadingQueryParams *params) {
    ReadingFilter filter = {0};

    if (params->equipment_id != NULL) {
        int value;
        if (parse_int_strict(params->equipment_id, &value) != 0 || value <= 0) {
            return make_error(400, "Query parameter 'equipment_id' must be a positive integer");
        }
        filter.has_equipment_id = 1;
        filter.equipment_id = value;
    }

    if (params->sensor_type != NULL) {
        if (!is_valid_sensor_type(params->sensor_type)) {
            return make_error(400, "Query parameter 'sensor_type' must be one of: temperature, vibration, rotational_speed");
        }
        filter.sensor_type = params->sensor_type;
    }

    filter.from_time = params->from_time;
    filter.to_time = params->to_time;

    if (params->limit != NULL) {
        int value;
        if (parse_int_strict(params->limit, &value) != 0 || value <= 0) {
            return make_error(400, "Query parameter 'limit' must be a positive integer");
        }
        filter.limit = value;
    }

    char errbuf[256];
    PGconn *conn = db_connect(g_conninfo, errbuf, sizeof(errbuf));
    if (conn == NULL) {
        return make_error(500, "Failed to connect to the database");
    }

    ReadingList list = {0};
    int rc = db_list_readings(conn, &filter, &list, errbuf, sizeof(errbuf));
    db_disconnect(conn);

    if (rc != 0) {
        return make_error(400, errbuf);
    }

    struct json_object *body = json_readings_array(&list);
    reading_list_free(&list);
    return make_response(200, body);
}

ApiResponse handle_create_reading(const char *json_body) {
    if (json_body == NULL || json_body[0] == '\0') {
        return make_error(400, "Request body must be a JSON object");
    }

    struct json_object *root = json_tokener_parse(json_body);
    if (root == NULL || !json_object_is_type(root, json_type_object)) {
        if (root != NULL) {
            json_object_put(root);
        }
        return make_error(400, "Request body must be valid JSON");
    }

    struct json_object *equipment_id_obj = NULL;
    struct json_object *sensor_type_obj = NULL;
    struct json_object *value_obj = NULL;
    struct json_object *unit_obj = NULL;
    struct json_object *recorded_at_obj = NULL;

    int has_equipment_id = json_object_object_get_ex(root, "equipment_id", &equipment_id_obj);
    int has_sensor_type = json_object_object_get_ex(root, "sensor_type", &sensor_type_obj);
    int has_value = json_object_object_get_ex(root, "value", &value_obj);
    int has_unit = json_object_object_get_ex(root, "unit", &unit_obj);
    int has_recorded_at = json_object_object_get_ex(root, "recorded_at", &recorded_at_obj);

    if (!has_equipment_id || !json_object_is_type(equipment_id_obj, json_type_int)) {
        json_object_put(root);
        return make_error(400, "Field 'equipment_id' is required and must be an integer");
    }

    if (!has_sensor_type || !json_object_is_type(sensor_type_obj, json_type_string) ||
        !is_valid_sensor_type(json_object_get_string(sensor_type_obj))) {
        json_object_put(root);
        return make_error(400, "Field 'sensor_type' is required and must be one of: temperature, vibration, rotational_speed");
    }

    if (!has_value ||
        !(json_object_is_type(value_obj, json_type_double) || json_object_is_type(value_obj, json_type_int))) {
        json_object_put(root);
        return make_error(400, "Field 'value' is required and must be a number");
    }

    int equipment_id = json_object_get_int(equipment_id_obj);
    if (equipment_id <= 0) {
        json_object_put(root);
        return make_error(400, "Field 'equipment_id' must be a positive integer");
    }

    const char *sensor_type = json_object_get_string(sensor_type_obj);
    double value = json_object_get_double(value_obj);
    const char *unit = (has_unit && json_object_is_type(unit_obj, json_type_string))
                            ? json_object_get_string(unit_obj)
                            : default_unit_for(sensor_type);
    const char *recorded_at = (has_recorded_at && json_object_is_type(recorded_at_obj, json_type_string))
                                   ? json_object_get_string(recorded_at_obj)
                                   : NULL;

    char errbuf[256];
    PGconn *conn = db_connect(g_conninfo, errbuf, sizeof(errbuf));
    if (conn == NULL) {
        json_object_put(root);
        return make_error(500, "Failed to connect to the database");
    }

    SensorReading created;
    int rc = db_insert_reading(conn, equipment_id, sensor_type, value, unit, recorded_at,
                                &created, errbuf, sizeof(errbuf));
    db_disconnect(conn);
    json_object_put(root);

    if (rc != 0) {
        if (strstr(errbuf, "foreign key") != NULL) {
            return make_error(404, "Equipment with the given id does not exist");
        }
        if (strstr(errbuf, "invalid input syntax for type timestamp") != NULL) {
            return make_error(400, "Field 'recorded_at' must be a valid ISO 8601 timestamp");
        }
        return make_error(400, errbuf);
    }

    return make_response(201, json_reading_object(&created));
}

ApiResponse handle_get_stats(const StatsQueryParams *params) {
    int has_equipment_id = 0;
    int equipment_id = 0;

    if (params->equipment_id != NULL) {
        if (parse_int_strict(params->equipment_id, &equipment_id) != 0 || equipment_id <= 0) {
            return make_error(400, "Query parameter 'equipment_id' must be a positive integer");
        }
        has_equipment_id = 1;
    }

    if (params->sensor_type != NULL && !is_valid_sensor_type(params->sensor_type)) {
        return make_error(400, "Query parameter 'sensor_type' must be one of: temperature, vibration, rotational_speed");
    }

    char errbuf[256];
    PGconn *conn = db_connect(g_conninfo, errbuf, sizeof(errbuf));
    if (conn == NULL) {
        return make_error(500, "Failed to connect to the database");
    }

    StatsList list = {0};
    int rc = db_get_stats(conn, has_equipment_id, equipment_id, params->sensor_type, &list,
                           errbuf, sizeof(errbuf));
    db_disconnect(conn);

    if (rc != 0) {
        return make_error(500, errbuf);
    }

    struct json_object *body = json_stats_array(&list);
    stats_list_free(&list);
    return make_response(200, body);
}

ApiResponse handle_not_found(void) {
    return make_error(404, "The requested resource was not found");
}
