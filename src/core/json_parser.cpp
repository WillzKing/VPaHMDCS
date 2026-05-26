#include "core/json_parser.h"
#include "db/pgsql_minimal.h"
#include "logging.h"

namespace {
    template<typename T>
    std::string IntOrEmpty(int v) { return v >= 0 ? std::to_string(v) : std::string(); }
    
    void ParseLocationObj(const json& loc, LocationInfo& out, long long defaultTs) {
        out.latitude  = loc.value("latitude", loc.value("Latitude", 0.0));
        out.longitude = loc.value("longitude", loc.value("Longitude", 0.0));
        out.altitude  = loc.value("altitude", loc.value("Altitude", 0.0));
        out.accuracy  = (float)loc.value("accuracy", loc.value("Accuracy", 0.0f));
        out.timestamp = loc.value("time", loc.value("Time", defaultTs));
    }
}

void ParseLocation(const json& j, RuntimeState* st) {
    auto& loc = st->currentUser.location;
    
    if (j.contains("location") && !j["location"].is_null()) {
        ParseLocationObj(j["location"], loc, j.value("timestamp", 0LL));
    } else if (j.contains("loc") && !j["loc"].is_null()) {
        ParseLocationObj(j["loc"], loc, j.value("timestamp", 0LL));
    }

    LogInfo(std::string("Parsed location: lat=") + std::to_string(loc.latitude) + " lon=" + std::to_string(loc.longitude) + " alt=" + std::to_string(loc.altitude) + " acc=" + std::to_string(loc.accuracy));
    st->currentUser.trajectory.push_back(loc);
    if (st->currentUser.trajectory.size() > 5000)
        st->currentUser.trajectory.erase(st->currentUser.trajectory.begin());
}

void ParseNetworkUsage(const json& j, RuntimeState* st) {
    auto it = j.find("networkUsage");
    if (it == j.end() || !it->is_object()) return;
    
    auto& user = st->currentUser;
    user.totalBytesSent     = it->value("totalBytesSent", 0LL);
    user.totalBytesReceived = it->value("totalBytesReceived", 0LL);
    user.topTrafficApps.clear();
    
    auto apps = it->find("topApps");
    if (apps != it->end() && apps->is_array()) {
        struct AppInfo { std::string pkg; long long bytes; };
        std::vector<AppInfo> appInfos; appInfos.reserve(apps->size());
        std::vector<double> values; values.reserve(apps->size());
        for (const auto& app : *apps) {
            std::string pkg = app.value("packageName", std::string("unknown"));
            long long b = app.value("bytes", 0LL);
            appInfos.push_back({pkg, b});
            values.push_back((double)b);
        }
        if (!values.empty()) {
            double mean = 0.0;
            for (double v : values) mean += v; mean /= (double)values.size();
            double var = 0.0; for (double v : values) var += (v - mean) * (v - mean); var /= (double)values.size();
            double sigma = std::sqrt(var);
            double threshold = mean + 2.0 * sigma;
            for (const auto& ai : appInfos) {
                if ((double)ai.bytes >= threshold)
                    user.topTrafficApps.emplace_back(ai.pkg + " : " + std::to_string(ai.bytes) + " (>=mean+2σ)");
                else
                    user.topTrafficApps.emplace_back(ai.pkg + " : " + std::to_string(ai.bytes));
            }
            LogInfo("Parsed networkUsage: apps=" + std::to_string(values.size()) + " mean=" + std::to_string(mean) + " sigma=" + std::to_string(sigma));
        }
    }
}

MobileNetworkInfo ParseCell(const json& c) {
    MobileNetworkInfo n;
    n.networkType   = c.value("type", "");
    n.cellIdentity  = c.value("cellId", c.value("nci", std::string("")));
    n.mcc = c.value("mcc", ""); n.mnc = c.value("mnc", "");
    n.pci           = c.value("pci", -1);
    n.rsrp          = c.value("rsrp", -140);
    n.rsrq          = c.value("rsrq", 0);
    n.rssi          = c.value("rssi", 0);
    n.earfcn        = c.value("earfcn",  -1);
    n.nrarfcn       = c.value("nrarfcn", -1);
    n.arfcn         = c.value("arfcn",   -1);
    n.timingAdvance = c.value("timingAdvance", 0);
    n.cqi           = c.value("cqi",  0);
    n.sinr          = c.value("sinr", 0.0);
    try { n.tac = std::stoi(c.value("tac", std::string("0"))); } catch (...) { n.tac = 0; }
    
    if (n.cellIdentity.empty() || n.cellIdentity == "2147483647") {
        if (c.contains("CellIdentityLte")) {
            auto& ci = c["CellIdentityLte"];
            n.cellIdentity = ci.value("CellIdentity", n.cellIdentity);
            n.mcc = ci.value("MCC", n.mcc);
            n.mnc = ci.value("MNC", n.mnc);
            n.pci = ci.value("PCI", n.pci);
            n.tac = ci.value("TAC", n.tac);
            n.earfcn = ci.value("EARFCN", n.earfcn);
            n.band = ci.value("Band", n.band);
        }
    }
    if (c.contains("CellSignalStrengthLte")) {
        auto& ss = c["CellSignalStrengthLte"];
        n.rsrp = ss.value("RSRP", n.rsrp);
        n.rsrq = ss.value("RSRQ", n.rsrq);
        n.rssi = ss.value("RSSI", n.rssi);
        n.timingAdvance = ss.value("Timing_Advance", n.timingAdvance);
        n.cqi = ss.value("CQI", n.cqi);
    }
    return n;
}

DbSignalRow MakeDbRow(const MobileNetworkInfo& n, const LocationInfo& loc) {
    DbSignalRow r{};
    r.imei = "android";
    r.lat  = std::to_string(loc.latitude);  r.lon = std::to_string(loc.longitude);
    r.alt  = std::to_string(loc.altitude);  r.accuracy = std::to_string(loc.accuracy);
    r.ts   = std::to_string(loc.timestamp);
    r.network_type = n.networkType;
    r.mcc = n.mcc; r.mnc = n.mnc; r.cell_id = n.cellIdentity;
    r.tac  = std::to_string(n.tac);   r.pci  = std::to_string(n.pci);
    r.rsrp = std::to_string(n.rsrp);  r.rsrq = std::to_string(n.rsrq);
    r.rssi = std::to_string(n.rssi);  r.sinr = "0";
    r.earfcn = IntOrEmpty<int>(n.earfcn); r.nrarfcn = IntOrEmpty<int>(n.nrarfcn); r.arfcn = IntOrEmpty<int>(n.arfcn);
    return r;
}

void AppendSignalPoint(RuntimeState* st, const MobileNetworkInfo& n) {
    const std::string key = n.networkType + "_" + n.cellIdentity
                          + "_PCI_" + std::to_string(n.pci)
                          + "_TAC_" + std::to_string(n.tac);

    auto [it, inserted] = st->seriesIndex.emplace(key, st->signalSeries.size());
    if (inserted) {
        auto& s = st->signalSeries.emplace_back();
        s.cellIdentity = key;
        s.timestamps.reserve(2000);
        s.rsrp_values.reserve(2000);
        s.rsrq_values.reserve(2000);
        s.rssi_values.reserve(2000);
    }

    auto& s  = st->signalSeries[it->second];
    double ts = (double)st->currentUser.location.timestamp;
    if (!s.timestamps.empty() && ts <= s.timestamps.back()) ts = s.timestamps.back() + 1.0;

    s.timestamps.push_back(ts);
    s.rsrp_values.push_back((double)n.rsrp);
    s.rsrq_values.push_back((double)n.rsrq);
    s.rssi_values.push_back((double)n.rssi);

    if (st->onSignalStrengthChange) st->onSignalStrengthChange(n);

    if (s.timestamps.size() > 2000) {
        const size_t drop = s.timestamps.size() - 1800;
        s.timestamps.erase  (s.timestamps.begin(),   s.timestamps.begin()   + (ptrdiff_t)drop);
        s.rsrp_values.erase (s.rsrp_values.begin(),  s.rsrp_values.begin()  + (ptrdiff_t)drop);
        s.rsrq_values.erase (s.rsrq_values.begin(),  s.rsrq_values.begin()  + (ptrdiff_t)drop);
        s.rssi_values.erase (s.rssi_values.begin(),  s.rssi_values.begin()  + (ptrdiff_t)drop);
    }
}

void ApplyPacket(RuntimeState* st, const std::string& raw, bool writeDb, PgSqlMinimal& db) {
    const auto j = json::parse(raw);
    ParseLocation(j, st);
    ParseNetworkUsage(j, st);
    st->currentUser.mobileNetworks.clear();
    
    std::vector<json> cells;
    if (j.contains("telephony") && j["telephony"].is_array()) {
        for (const auto& c : j["telephony"]) cells.push_back(c);
    } else if (j.contains("mobile_network_data_list") && 
               j["mobile_network_data_list"].contains("MobileNetworks")) {
        for (const auto& c : j["mobile_network_data_list"]["MobileNetworks"]) cells.push_back(c);
    }
    
    for (const auto& c : cells) {
        auto n = ParseCell(c);
        st->currentUser.mobileNetworks.push_back(n);
        if (writeDb) db.insertRow(MakeDbRow(n, st->currentUser.location));
        AppendSignalPoint(st, n);

        MapPoint mp;
        mp.latitude = st->currentUser.location.latitude;
        mp.longitude = st->currentUser.location.longitude;
        mp.altitude = st->currentUser.location.altitude;
        mp.rsrp = n.rsrp;
        mp.rsrq = n.rsrq;
        mp.rssi = n.rssi;
        mp.earfcn = n.earfcn;
        mp.pci = n.pci;
        mp.time = st->currentUser.location.timestamp;
        st->heatmapPoints.push_back(mp);
        if (st->heatmapPoints.size() > 10000) st->heatmapPoints.erase(st->heatmapPoints.begin());
    }
}

bool ParseControlCommand(const std::string& msg, bool& gps, bool& lte, bool& nr) {
    try {
        json cmd = json::parse(msg);
        if (cmd.value("type", "") != "control") return false;
        gps = cmd.value("gps", gps);
        lte = cmd.value("lte", lte);
        nr  = cmd.value("nr", nr);
        return true;
    } catch (...) { return false; }
}
