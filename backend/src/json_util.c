#include "json_util.h"

struct json_object *json_error(const char *message) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "error", json_object_new_string(message));
    return obj;
}

struct json_object *json_reading_object(const SensorReading *reading) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "id", json_object_new_int64(reading->id));
    json_object_object_add(obj, "equipment_id", json_object_new_int(reading->equipment_id));
    if (reading->equipment_name[0] != '\0') {
        json_object_object_add(obj, "equipment_name", json_object_new_string(reading->equipment_name));
    }
    json_object_object_add(obj, "sensor_type", json_object_new_string(reading->sensor_type));
    json_object_object_add(obj, "value", json_object_new_double(reading->value));
    json_object_object_add(obj, "unit", json_object_new_string(reading->unit));
    json_object_object_add(obj, "recorded_at", json_object_new_string(reading->recorded_at));
    json_object_object_add(obj, "created_at", json_object_new_string(reading->created_at));
    return obj;
}

struct json_object *json_readings_array(const ReadingList *list) {
    struct json_object *arr = json_object_new_array();
    for (size_t i = 0; i < list->count; i++) {
        json_object_array_add(arr, json_reading_object(&list->items[i]));
    }
    struct json_object *wrapper = json_object_new_object();
    json_object_object_add(wrapper, "count", json_object_new_int((int) list->count));
    json_object_object_add(wrapper, "readings", arr);
    return wrapper;
}

struct json_object *json_equipment_object(const Equipment *equipment) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "id", json_object_new_int(equipment->id));
    json_object_object_add(obj, "name", json_object_new_string(equipment->name));
    json_object_object_add(obj, "equipment_type", json_object_new_string(equipment->equipment_type));
    if (equipment->location[0] != '\0') {
        json_object_object_add(obj, "location", json_object_new_string(equipment->location));
    }
    json_object_object_add(obj, "created_at", json_object_new_string(equipment->created_at));
    return obj;
}

struct json_object *json_equipment_array(const EquipmentList *list) {
    struct json_object *arr = json_object_new_array();
    for (size_t i = 0; i < list->count; i++) {
        json_object_array_add(arr, json_equipment_object(&list->items[i]));
    }
    struct json_object *wrapper = json_object_new_object();
    json_object_object_add(wrapper, "count", json_object_new_int((int) list->count));
    json_object_object_add(wrapper, "equipment", arr);
    return wrapper;
}

struct json_object *json_stats_object(const SensorStats *stats) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "equipment_id", json_object_new_int(stats->equipment_id));
    json_object_object_add(obj, "equipment_name", json_object_new_string(stats->equipment_name));
    json_object_object_add(obj, "sensor_type", json_object_new_string(stats->sensor_type));
    json_object_object_add(obj, "average", json_object_new_double(stats->avg_value));
    json_object_object_add(obj, "minimum", json_object_new_double(stats->min_value));
    json_object_object_add(obj, "maximum", json_object_new_double(stats->max_value));
    json_object_object_add(obj, "sample_count", json_object_new_int64(stats->sample_count));
    return obj;
}

struct json_object *json_stats_array(const StatsList *list) {
    struct json_object *arr = json_object_new_array();
    for (size_t i = 0; i < list->count; i++) {
        json_object_array_add(arr, json_stats_object(&list->items[i]));
    }
    struct json_object *wrapper = json_object_new_object();
    json_object_object_add(wrapper, "count", json_object_new_int((int) list->count));
    json_object_object_add(wrapper, "stats", arr);
    return wrapper;
}
