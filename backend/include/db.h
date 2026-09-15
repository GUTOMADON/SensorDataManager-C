#ifndef SDM_DB_H
#define SDM_DB_H

#include <stddef.h>
#include <libpq-fe.h>
#include "models.h"

/* Thin data-access layer around libpq. Every query is executed with
 * PQexecParams and bound parameters, so caller-supplied values are never
 * concatenated into SQL text and SQL injection is not possible through
 * this layer. */

typedef struct {
    SensorReading *items;
    size_t count;
} ReadingList;

typedef struct {
    Equipment *items;
    size_t count;
} EquipmentList;

typedef struct {
    SensorStats *items;
    size_t count;
} StatsList;

typedef struct {
    int has_equipment_id;
    int equipment_id;
    const char *sensor_type; /* NULL if not filtered */
    const char *from_time;   /* ISO 8601 timestamp, or NULL */
    const char *to_time;     /* ISO 8601 timestamp, or NULL */
    int limit;                /* 0 means "use default" */
} ReadingFilter;

/* Opens a new connection using the given libpq connection string.
 * Returns NULL and writes a message into errbuf on failure. */
PGconn *db_connect(const char *conninfo, char *errbuf, size_t errbuf_len);
void db_disconnect(PGconn *conn);

int db_insert_reading(PGconn *conn, int equipment_id, const char *sensor_type,
                       double value, const char *unit, const char *recorded_at,
                       SensorReading *out, char *errbuf, size_t errbuf_len);

int db_list_readings(PGconn *conn, const ReadingFilter *filter, ReadingList *out,
                      char *errbuf, size_t errbuf_len);

int db_list_equipment(PGconn *conn, EquipmentList *out, char *errbuf, size_t errbuf_len);

int db_get_stats(PGconn *conn, int has_equipment_id, int equipment_id,
                  const char *sensor_type, StatsList *out,
                  char *errbuf, size_t errbuf_len);

void reading_list_free(ReadingList *list);
void equipment_list_free(EquipmentList *list);
void stats_list_free(StatsList *list);

#endif /* SDM_DB_H */
