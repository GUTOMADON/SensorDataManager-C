#ifndef SDM_JSON_UTIL_H
#define SDM_JSON_UTIL_H

#include <json-c/json.h>
#include "models.h"
#include "db.h"

/* Builds json-c objects out of domain structures. The caller owns the
 * returned object and must release it with json_object_put(). */

struct json_object *json_error(const char *message);

struct json_object *json_reading_object(const SensorReading *reading);
struct json_object *json_readings_array(const ReadingList *list);

struct json_object *json_equipment_object(const Equipment *equipment);
struct json_object *json_equipment_array(const EquipmentList *list);

struct json_object *json_stats_object(const SensorStats *stats);
struct json_object *json_stats_array(const StatsList *list);

#endif /* SDM_JSON_UTIL_H */
