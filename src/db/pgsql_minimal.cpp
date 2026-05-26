#include "db/pgsql_minimal.h"
#include "logging.h"
#include <libpq-fe.h>
#include <iostream>

bool PgSqlMinimal::connect(const DbConfig& cfg) {
    std::string info = "host=" + cfg.host + " port=" + cfg.port + " dbname=" + cfg.db +
                       " user=" + cfg.user + " password=" + cfg.pass;
    conn_ = PQconnectdb(info.c_str());
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

void PgSqlMinimal::disconnect() { if (conn_) { PQfinish(conn_); conn_ = nullptr; } }

bool PgSqlMinimal::ensureTable() {
    const char* q = "CREATE TABLE IF NOT EXISTS user_equipment(" \
                    "imei TEXT, lat DOUBLE PRECISION, lon DOUBLE PRECISION, alt DOUBLE PRECISION, accuracy DOUBLE PRECISION," \
                    "ts BIGINT, network_type TEXT, mcc TEXT, mnc TEXT, cell_id TEXT, tac TEXT, lac TEXT, nci TEXT," \
                    "earfcn TEXT, nrarfcn TEXT, arfcn TEXT, pci INT, rsrp INT, rsrq INT, rssi INT, sinr INT)";
    PGresult* r = PQexec(conn_, q);
    bool ok = r && PQresultStatus(r) == PGRES_COMMAND_OK;
    PQclear(r);
    return ok;
}

bool PgSqlMinimal::insertRow(const DbSignalRow& x) {
    const char* vals[] = {x.imei.c_str(), x.lat.c_str(), x.lon.c_str(), x.alt.c_str(), x.accuracy.c_str(), x.ts.c_str(),
                          x.network_type.c_str(), x.mcc.c_str(), x.mnc.c_str(), x.cell_id.c_str(), x.tac.c_str(), x.lac.c_str(), x.nci.c_str(),
                          x.earfcn.c_str(), x.nrarfcn.c_str(), x.arfcn.c_str(), x.pci.c_str(), x.rsrp.c_str(), x.rsrq.c_str(), x.rssi.c_str(), x.sinr.c_str()};
    const char* q = "INSERT INTO user_equipment(imei,lat,lon,alt,accuracy,ts,network_type,mcc,mnc,cell_id,tac,lac,nci,earfcn,nrarfcn,arfcn,pci,rsrp,rsrq,rssi,sinr) "
                    "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21)";
    PGresult* r = PQexecParams(conn_, q, 21, nullptr, vals, nullptr, nullptr, 0);
    bool ok = r && PQresultStatus(r) == PGRES_COMMAND_OK;
    if (!ok) {
        if (conn_) LogError(std::string("DB insert failed: ") + PQerrorMessage(conn_));
    }
    PQclear(r);
    return ok;
}
