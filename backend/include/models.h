#ifndef SDM_MODELS_H
#define SDM_MODELS_H

/* Plain data structures shared between the database layer, the handlers,
 * and the JSON serialization layer. Text fields use fixed-size buffers
 * since equipment and sensor metadata are short, bounded strings. */

#define SDM_NAME_LEN 128
#define SDM_TYPE_LEN 32
#define SDM_UNIT_LEN 16
#define SDM_TIMESTAMP_LEN 64

typedef struct {
    int id;
    char name[SDM_NAME_LEN];
    char equipment_type[SDM_TYPE_LEN];
    char location[SDM_NAME_LEN];
    char created_at[SDM_TIMESTAMP_LEN];
} Equipment;

typedef struct {
    long id;
    int equipment_id;
    char equipment_name[SDM_NAME_LEN];
    char sensor_type[SDM_TYPE_LEN];
    double value;
    char unit[SDM_UNIT_LEN];
    char recorded_at[SDM_TIMESTAMP_LEN];
    char created_at[SDM_TIMESTAMP_LEN];
} SensorReading;

typedef struct {
    int equipment_id;
    char equipment_name[SDM_NAME_LEN];
    char sensor_type[SDM_TYPE_LEN];
    double avg_value;
    double min_value;
    double max_value;
    long sample_count;
} SensorStats;

#endif /* SDM_MODELS_H */
