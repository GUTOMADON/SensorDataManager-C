#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Format used to render all timestamps returned to API clients: a fixed
 * ISO 8601 UTC string, e.g. 2026-09-15T14:30:00Z. */
#define TIMESTAMP_SELECT_EXPR(column) \
    "to_char((" column ") AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"')"

PGconn *db_connect(const char *conninfo, char *errbuf, size_t errbuf_len) {
    PGconn *conn = PQconnectdb(conninfo);

    if (conn == NULL) {
        snprintf(errbuf, errbuf_len, "Could not allocate a database connection");
        return NULL;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        snprintf(errbuf, errbuf_len, "%s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    return conn;
}

void db_disconnect(PGconn *conn) {
    if (conn != NULL) {
        PQfinish(conn);
    }
}

static void fill_reading_from_row(PGresult *res, int row, SensorReading *out) {
    memset(out, 0, sizeof(*out));
    out->id = atol(PQgetvalue(res, row, 0));
    out->equipment_id = atoi(PQgetvalue(res, row, 1));
    snprintf(out->equipment_name, sizeof(out->equipment_name), "%s", PQgetvalue(res, row, 2));
    snprintf(out->sensor_type, sizeof(out->sensor_type), "%s", PQgetvalue(res, row, 3));
    out->value = atof(PQgetvalue(res, row, 4));
    snprintf(out->unit, sizeof(out->unit), "%s", PQgetvalue(res, row, 5));
    snprintf(out->recorded_at, sizeof(out->recorded_at), "%s", PQgetvalue(res, row, 6));
    snprintf(out->created_at, sizeof(out->created_at), "%s", PQgetvalue(res, row, 7));
}

int db_insert_reading(PGconn *conn, int equipment_id, const char *sensor_type,
                       double value, const char *unit, const char *recorded_at,
                       SensorReading *out, char *errbuf, size_t errbuf_len) {
    char equipment_id_str[16];
    char value_str[64];

    snprintf(equipment_id_str, sizeof(equipment_id_str), "%d", equipment_id);
    snprintf(value_str, sizeof(value_str), "%.6f", value);

    const char *params[5];
    params[0] = equipment_id_str;
    params[1] = sensor_type;
    params[2] = value_str;
    params[3] = unit;
    params[4] = (recorded_at != NULL && recorded_at[0] != '\0') ? recorded_at : NULL;

    const char *sql =
        "INSERT INTO sensor_readings (equipment_id, sensor_type, value, unit, recorded_at) "
        "VALUES ($1, $2, $3, $4, COALESCE($5::timestamptz, now())) "
        "RETURNING id, equipment_id, "
        "(SELECT name FROM equipment WHERE id = $1), "
        "sensor_type, value, unit, "
        TIMESTAMP_SELECT_EXPR("recorded_at") ", "
        TIMESTAMP_SELECT_EXPR("created_at");

    PGresult *res = PQexecParams(conn, sql, 5, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        snprintf(errbuf, errbuf_len, "%s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    fill_reading_from_row(res, 0, out);
    PQclear(res);
    return 0;
}

int db_list_readings(PGconn *conn, const ReadingFilter *filter, ReadingList *out,
                      char *errbuf, size_t errbuf_len) {
    char sql[1536];
    char equipment_id_str[16];
    char limit_str[16];
    const char *params[5];
    int nparams = 0;
    char clause[80];

    snprintf(sql, sizeof(sql),
        "SELECT r.id, r.equipment_id, e.name, r.sensor_type, r.value, r.unit, "
        TIMESTAMP_SELECT_EXPR("r.recorded_at") ", "
        TIMESTAMP_SELECT_EXPR("r.created_at") " "
        "FROM sensor_readings r JOIN equipment e ON e.id = r.equipment_id WHERE 1=1");

    if (filter->has_equipment_id) {
        nparams++;
        snprintf(equipment_id_str, sizeof(equipment_id_str), "%d", filter->equipment_id);
        params[nparams - 1] = equipment_id_str;
        snprintf(clause, sizeof(clause), " AND r.equipment_id = $%d", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    if (filter->sensor_type != NULL) {
        nparams++;
        params[nparams - 1] = filter->sensor_type;
        snprintf(clause, sizeof(clause), " AND r.sensor_type = $%d", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    if (filter->from_time != NULL) {
        nparams++;
        params[nparams - 1] = filter->from_time;
        snprintf(clause, sizeof(clause), " AND r.recorded_at >= $%d::timestamptz", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    if (filter->to_time != NULL) {
        nparams++;
        params[nparams - 1] = filter->to_time;
        snprintf(clause, sizeof(clause), " AND r.recorded_at <= $%d::timestamptz", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }

    strncat(sql, " ORDER BY r.recorded_at DESC", sizeof(sql) - strlen(sql) - 1);

    int limit = filter->limit > 0 ? filter->limit : 100;
    if (limit > 1000) {
        limit = 1000;
    }
    nparams++;
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    params[nparams - 1] = limit_str;
    snprintf(clause, sizeof(clause), " LIMIT $%d", nparams);
    strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);

    PGresult *res = PQexecParams(conn, sql, nparams, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        snprintf(errbuf, errbuf_len, "%s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int nrows = PQntuples(res);
    out->count = (size_t) nrows;
    out->items = NULL;

    if (nrows > 0) {
        out->items = calloc((size_t) nrows, sizeof(SensorReading));
        if (out->items == NULL) {
            snprintf(errbuf, errbuf_len, "Out of memory while building reading list");
            PQclear(res);
            return -1;
        }
        for (int i = 0; i < nrows; i++) {
            fill_reading_from_row(res, i, &out->items[i]);
        }
    }

    PQclear(res);
    return 0;
}

int db_list_equipment(PGconn *conn, EquipmentList *out, char *errbuf, size_t errbuf_len) {
    const char *sql =
        "SELECT id, name, equipment_type, location, " TIMESTAMP_SELECT_EXPR("created_at") " "
        "FROM equipment ORDER BY name";

    PGresult *res = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        snprintf(errbuf, errbuf_len, "%s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int nrows = PQntuples(res);
    out->count = (size_t) nrows;
    out->items = NULL;

    if (nrows > 0) {
        out->items = calloc((size_t) nrows, sizeof(Equipment));
        if (out->items == NULL) {
            snprintf(errbuf, errbuf_len, "Out of memory while building equipment list");
            PQclear(res);
            return -1;
        }
        for (int i = 0; i < nrows; i++) {
            Equipment *e = &out->items[i];
            memset(e, 0, sizeof(*e));
            e->id = atoi(PQgetvalue(res, i, 0));
            snprintf(e->name, sizeof(e->name), "%s", PQgetvalue(res, i, 1));
            snprintf(e->equipment_type, sizeof(e->equipment_type), "%s", PQgetvalue(res, i, 2));
            if (!PQgetisnull(res, i, 3)) {
                snprintf(e->location, sizeof(e->location), "%s", PQgetvalue(res, i, 3));
            }
            snprintf(e->created_at, sizeof(e->created_at), "%s", PQgetvalue(res, i, 4));
        }
    }

    PQclear(res);
    return 0;
}

int db_get_stats(PGconn *conn, int has_equipment_id, int equipment_id,
                  const char *sensor_type, StatsList *out,
                  char *errbuf, size_t errbuf_len) {
    char sql[1024];
    char equipment_id_str[16];
    const char *params[2];
    int nparams = 0;
    char clause[80];

    snprintf(sql, sizeof(sql),
        "SELECT r.equipment_id, e.name, r.sensor_type, "
        "AVG(r.value), MIN(r.value), MAX(r.value), COUNT(*) "
        "FROM sensor_readings r JOIN equipment e ON e.id = r.equipment_id WHERE 1=1");

    if (has_equipment_id) {
        nparams++;
        snprintf(equipment_id_str, sizeof(equipment_id_str), "%d", equipment_id);
        params[nparams - 1] = equipment_id_str;
        snprintf(clause, sizeof(clause), " AND r.equipment_id = $%d", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    if (sensor_type != NULL) {
        nparams++;
        params[nparams - 1] = sensor_type;
        snprintf(clause, sizeof(clause), " AND r.sensor_type = $%d", nparams);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }

    strncat(sql, " GROUP BY r.equipment_id, e.name, r.sensor_type ORDER BY e.name, r.sensor_type",
            sizeof(sql) - strlen(sql) - 1);

    PGresult *res = PQexecParams(conn, sql, nparams, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        snprintf(errbuf, errbuf_len, "%s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int nrows = PQntuples(res);
    out->count = (size_t) nrows;
    out->items = NULL;

    if (nrows > 0) {
        out->items = calloc((size_t) nrows, sizeof(SensorStats));
        if (out->items == NULL) {
            snprintf(errbuf, errbuf_len, "Out of memory while building stats list");
            PQclear(res);
            return -1;
        }
        for (int i = 0; i < nrows; i++) {
            SensorStats *s = &out->items[i];
            memset(s, 0, sizeof(*s));
            s->equipment_id = atoi(PQgetvalue(res, i, 0));
            snprintf(s->equipment_name, sizeof(s->equipment_name), "%s", PQgetvalue(res, i, 1));
            snprintf(s->sensor_type, sizeof(s->sensor_type), "%s", PQgetvalue(res, i, 2));
            s->avg_value = atof(PQgetvalue(res, i, 3));
            s->min_value = atof(PQgetvalue(res, i, 4));
            s->max_value = atof(PQgetvalue(res, i, 5));
            s->sample_count = atol(PQgetvalue(res, i, 6));
        }
    }

    PQclear(res);
    return 0;
}

void reading_list_free(ReadingList *list) {
    if (list == NULL) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

void equipment_list_free(EquipmentList *list) {
    if (list == NULL) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

void stats_list_free(StatsList *list) {
    if (list == NULL) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}
