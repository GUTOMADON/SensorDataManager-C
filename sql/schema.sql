-- SensorDataManager-C database schema
--
-- Defines two tables:
--   equipment       : the physical assets being monitored (pumps, motors, etc.)
--   sensor_readings : individual time-stamped measurements collected from equipment
--
-- Run this file once against a fresh PostgreSQL database before running seed.sql.

BEGIN;

CREATE TABLE IF NOT EXISTS equipment (
    id              SERIAL PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    equipment_type  TEXT NOT NULL,
    location        TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS sensor_readings (
    id              BIGSERIAL PRIMARY KEY,
    equipment_id    INTEGER NOT NULL REFERENCES equipment(id) ON DELETE CASCADE,
    sensor_type     TEXT NOT NULL CHECK (sensor_type IN ('temperature', 'vibration', 'rotational_speed')),
    value           DOUBLE PRECISION NOT NULL,
    unit            TEXT NOT NULL,
    recorded_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Speeds up the common query patterns used by the API: filtering by equipment,
-- filtering by sensor type, and filtering/sorting by time range.
CREATE INDEX IF NOT EXISTS idx_readings_equipment_id ON sensor_readings (equipment_id);
CREATE INDEX IF NOT EXISTS idx_readings_sensor_type ON sensor_readings (sensor_type);
CREATE INDEX IF NOT EXISTS idx_readings_recorded_at ON sensor_readings (recorded_at DESC);
CREATE INDEX IF NOT EXISTS idx_readings_equipment_sensor_time
    ON sensor_readings (equipment_id, sensor_type, recorded_at DESC);

COMMIT;
