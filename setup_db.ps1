# setup_db.ps1
$PG_BIN = "C:\Program Files\PostgreSQL\18\bin"
$DB_NAME = "backend_control"
$DB_USER = "postgres"

Write-Host "=== Создание базы данных ===" -ForegroundColor Cyan

# Создание БД
& "$PG_BIN\psql.exe" -U $DB_USER -c "CREATE DATABASE $DB_NAME;"

# Создание таблиц
& "$PG_BIN\psql.exe" -U $DB_USER -d $DB_NAME -c @"
CREATE TABLE IF NOT EXISTS location_history (
    id SERIAL PRIMARY KEY,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    altitude DOUBLE PRECISION DEFAULT 0,
    accuracy REAL DEFAULT 0,
    timestamp BIGINT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS telephony_history (
    id SERIAL PRIMARY KEY,
    cell_type VARCHAR(50) NOT NULL,
    cell_key VARCHAR(255) NOT NULL,
    mcc VARCHAR(10),
    mnc VARCHAR(10),
    cell_id VARCHAR(50),
    lac_tac VARCHAR(50),
    rsrp INTEGER,
    signal_level INTEGER,
    bandwidth INTEGER,
    timestamp BIGINT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS network_history (
    id SERIAL PRIMARY KEY,
    bytes_sent BIGINT DEFAULT 0,
    bytes_received BIGINT DEFAULT 0,
    package_name VARCHAR(255),
    timestamp BIGINT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_location_timestamp ON location_history(timestamp);
CREATE INDEX IF NOT EXISTS idx_telephony_timestamp ON telephony_history(timestamp);
CREATE INDEX IF NOT EXISTS idx_telephony_cell_key ON telephony_history(cell_key);
CREATE INDEX IF NOT EXISTS idx_network_timestamp ON network_history(timestamp);
"@

Write-Host "Database is ready!" -ForegroundColor Green