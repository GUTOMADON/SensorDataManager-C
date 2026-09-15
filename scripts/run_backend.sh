#!/usr/bin/env bash
# Builds and runs the SensorDataManager-C backend. Assumes the database has
# already been set up (see scripts/setup_db.sh) and that the SDM_DB_* /
# DATABASE_URL environment variables are configured if not using defaults.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="$SCRIPT_DIR/../backend"

cd "$BACKEND_DIR"
make
./bin/sensor_data_manager
