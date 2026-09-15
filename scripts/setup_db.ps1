# Creates the SensorDataManager-C database (if it does not already exist),
# applies the schema, and loads the sample dataset. This is the Windows
# PowerShell equivalent of scripts/setup_db.sh, for use when PostgreSQL's
# client tools (createdb, psql) are installed natively on Windows.
#
# Usage: powershell -File scripts/setup_db.ps1

$ErrorActionPreference = "Stop"

$DbName = if ($env:SDM_DB_NAME) { $env:SDM_DB_NAME } else { "sensor_data_manager" }
$DbUser = if ($env:SDM_DB_USER) { $env:SDM_DB_USER } else { "postgres" }
$DbHost = if ($env:SDM_DB_HOST) { $env:SDM_DB_HOST } else { "localhost" }
$DbPort = if ($env:SDM_DB_PORT) { $env:SDM_DB_PORT } else { "5432" }
$env:PGPASSWORD = if ($env:SDM_DB_PASSWORD) { $env:SDM_DB_PASSWORD } else { "postgres" }

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SqlDir = Join-Path $ScriptDir "..\sql"

Write-Host "Creating database '$DbName' if it does not already exist..."
& createdb -h $DbHost -p $DbPort -U $DbUser $DbName 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Database '$DbName' already exists, continuing."
}

Write-Host "Applying schema from $SqlDir\schema.sql ..."
& psql -h $DbHost -p $DbPort -U $DbUser -d $DbName -v ON_ERROR_STOP=1 -f "$SqlDir\schema.sql"
if ($LASTEXITCODE -ne 0) { throw "Failed to apply schema.sql" }

Write-Host "Loading sample data from $SqlDir\seed.sql ..."
& psql -h $DbHost -p $DbPort -U $DbUser -d $DbName -v ON_ERROR_STOP=1 -f "$SqlDir\seed.sql"
if ($LASTEXITCODE -ne 0) { throw "Failed to apply seed.sql" }

Write-Host "Database setup complete: $DbName is ready on $DbHost`:$DbPort."
