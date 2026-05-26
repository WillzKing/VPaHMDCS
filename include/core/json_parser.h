#pragma once
#include <nlohmann/json.hpp>
#include "core/runtime_state.h"

// Forward declaration
struct PgSqlMinimal;
struct DbSignalRow;

using json = nlohmann::json;

// ─── Парсинг местоположения ──────────────────────────────────────────────────
void ParseLocation(const json& j, RuntimeState* st);

// ─── Парсинг использования сети ──────────────────────────────────────────────
void ParseNetworkUsage(const json& j, RuntimeState* st);

// ─── Парсинг данных соты ─────────────────────────────────────────────────────
MobileNetworkInfo ParseCell(const json& c);

// ─── Создание строки для БД ──────────────────────────────────────────────────
DbSignalRow MakeDbRow(const MobileNetworkInfo& n, const LocationInfo& loc);

// ─── Добавление точки сигнала в серию ────────────────────────────────────────
void AppendSignalPoint(RuntimeState* st, const MobileNetworkInfo& n);

// ─── Применение пакета данных ────────────────────────────────────────────────
void ApplyPacket(RuntimeState* st, const std::string& raw, bool writeDb, PgSqlMinimal& db);

// ─── Парсинг команд управления ───────────────────────────────────────────────
bool ParseControlCommand(const std::string& msg, bool& gps, bool& lte, bool& nr);
