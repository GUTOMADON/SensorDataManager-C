#!/usr/bin/env bash
# Creates the SensorDataManager-C database (if it does not already exist),
# applies the schema, and loads the sample dataset.
#
# Requires the PostgreSQL client tools (createdb, psql) and a reachable
# PostgreSQL server. Configure the connection with the same SDM_DB_* / PG*
# environment variables documented in backend/README.md, or edit the
# defaults below.

set -euo pipefail

DB_NAME="${SDM_DB_NAME:-sensor_data_manager}"
DB_USER="${SDM_DB_USER:-postgres}"
DB_HOST="${SDM_DB_HOST:-localhost}"
DB_PORT="${SDM_DB_PORT:-5432}"
export PGPASSWORD="${SDM_DB_PASSWORD:-postgres}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SQL_DIR="$SCRIPT_DIR/../sql"

echo "Creating database '$DB_NAME' if it does not already exist..."
createdb -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" "$DB_NAME" 2>/dev/null || \
    echo "Database '$DB_NAME' already exists, continuing."

echo "Applying schema from $SQL_DIR/schema.sql ..."
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f "$SQL_DIR/schema.sql"

echo "Loading sample data from $SQL_DIR/seed.sql ..."
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f "$SQL_DIR/seed.sql"

echo "Database setup complete: $DB_NAME is ready on $DB_HOST:$DB_PORT."
