-- Sample data for SensorDataManager-C
--
-- Populates the equipment table with five representative industrial assets and
-- generates two days of hourly sensor readings (temperature, vibration, and
-- rotational speed) for each one, so the API and dashboard have realistic data
-- to display immediately after setup.
--
-- Run this file after sql/schema.sql has been applied.

BEGIN;

TRUNCATE TABLE sensor_readings RESTART IDENTITY CASCADE;
TRUNCATE TABLE equipment RESTART IDENTITY CASCADE;

INSERT INTO equipment (name, equipment_type, location) VALUES
    ('Pump-01',       'pump',       'Plant A - Line 1'),
    ('Motor-02',      'motor',      'Plant A - Line 1'),
    ('Compressor-03', 'compressor', 'Plant A - Line 2'),
    ('Conveyor-04',   'conveyor',   'Plant B - Packaging'),
    ('Turbine-05',    'turbine',    'Plant B - Power House');

-- Generate 48 hourly readings per equipment per sensor type (last 2 days).
-- Baseline values and noise ranges are chosen to look like plausible
-- industrial telemetry rather than fully random noise.
INSERT INTO sensor_readings (equipment_id, sensor_type, value, unit, recorded_at)
SELECT
    e.id,
    s.sensor_type,
    ROUND(
        (CASE s.sensor_type
            WHEN 'temperature'      THEN 55 + (random() * 15)
            WHEN 'vibration'        THEN 0.5 + (random() * 2.5)
            WHEN 'rotational_speed' THEN 1400 + (random() * 300)
        END)::numeric,
        2
    ) AS value,
    CASE s.sensor_type
        WHEN 'temperature'      THEN 'C'
        WHEN 'vibration'        THEN 'mm/s'
        WHEN 'rotational_speed' THEN 'rpm'
    END AS unit,
    now() - (h.hours_ago || ' hours')::interval AS recorded_at
FROM equipment e
CROSS JOIN (VALUES ('temperature'), ('vibration'), ('rotational_speed')) AS s(sensor_type)
CROSS JOIN generate_series(0, 47) AS h(hours_ago);

COMMIT;
