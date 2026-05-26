#pragma once
#include <string>
#include <vector>
#include <libpq-fe.h>

struct DbConfig {
    std::string host{"localhost"};
    std::string port{"5432"};
    std::string db{"backend_control"};
    std::string user{"postgres"};
    std::string pass{"2002"};
};

struct DbSignalRow {
    std::string imei;
    std::string lat, lon, alt, ts;
    std::string accuracy;
    std::string network_type;
    std::string mcc, mnc;
    std::string cell_id, tac, lac, nci;
    std::string earfcn, nrarfcn, arfcn;
    std::string pci, rsrp, rsrq, rssi, sinr;
};

class PgSqlMinimal {
public:
    bool connect(const DbConfig& cfg);
    void disconnect();
    bool ensureTable();
    bool insertRow(const DbSignalRow& r);
private:
    PGconn* conn_{nullptr};
};
