#include "app_server.h"
#include "app_utils.h"
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

using namespace std;
using json = nlohmann::json;

void run_server_thread(SystemStatus* shared_data, Database* db, DatabaseQueue* dbQueue, atomic<bool>* should_stop) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PULL);
    zmq::socket_t cmdSocket(context, ZMQ_PUSH);
    socket.set(zmq::sockopt::rcvtimeo, 100);
    socket.set(zmq::sockopt::rcvhwm, 5000);
    int attempt = 0;
    while (attempt < 5) {
        try {
            socket.bind("tcp://0.0.0.0:5558");
            cmdSocket.bind("tcp://0.0.0.0:5559");
            cout << "[ZMQ] Прослушивание портов 5558, 5559" << endl;
            break;
        } catch (...) {
            attempt++;
            this_thread::sleep_for(chrono::seconds(1));
        }
    }
    auto lastStats = chrono::steady_clock::now();
    int msgPerSec = 0;
    while (!should_stop->load()) {
        try {
            zmq::message_t msg;
            if (socket.recv(msg, zmq::recv_flags::none)) {
                long long nowEpoch = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
                shared_data->lastMessageTime.store(nowEpoch);
                shared_data->messageCount.fetch_add(1);
                msgPerSec++;
                string rcv(static_cast<char*>(msg.data()), msg.size());
                json j;
                try { j = json::parse(rcv, nullptr, false); if (j.is_discarded()) continue; } catch (...) { continue; }
                long long ts = j.value("timestamp", nowEpoch);
                string msgType = j.value("type", "unknown");
                {
                    lock_guard<mutex> lock(shared_data->mtx);
                    shared_data->has_connection = true;
                    if (msgType == "telephony_data" && j.contains("cells") && j["cells"].is_array()) {
                        shared_data->cells.clear();
                        for (auto& cell : j["cells"]) {
                            CellInfo info;
                            info.type = cell.value("type", "Unknown");
                            info.mcc = cell.value("mcc", ""); info.mnc = cell.value("mnc", ""); info.pci = cell.value("pci", -1);
                            if (info.type == "LTE") {
                                info.cellId = to_string(cell.value("cellId", 0)); info.tac = to_string(cell.value("tac", 0));
                                info.earfcn = cell.value("earfcn", -1); info.band = cell.value("band", -1);
                                info.rsrp = cell.value("rsrp", -140); info.rssi = cell.value("rssi", 0); info.sinr = cell.value("sinr", cell.value("rssnr", 0));
                                info.rsrq = cell.value("rsrq", 0); info.cqi = cell.value("cqi", 0); info.asuLevel = cell.value("asuLevel", 0); info.timingAdvance = cell.value("timingAdvance", 0);
                                info.signalStrength = info.rsrp;
                            } else if (info.type == "GSM") {
                                info.cellId = to_string(cell.value("cellId", 0)); info.lac = to_string(cell.value("lac", 0));
                                info.arfcn = cell.value("arfcn", -1); info.bsic = cell.value("bsic", -1);
                                info.rssi = cell.value("rssi", 0); info.dbm = cell.value("dbm", -140); info.timingAdvance = cell.value("timingAdvance", 0);
                                info.signalStrength = info.dbm;
                            } else if (info.type == "NR") {
                                info.type = "5G_NR";
                                info.nci = to_string(cell.value("nci", 0LL)); info.tac = to_string(cell.value("tac", 0));
                                info.pci = cell.value("pci", -1); info.nrarfcn = cell.value("nrarfcn", -1); info.band = cell.value("band", -1);
                                info.ssRsrp = cell.value("ssRsrp", -140); info.ssRsrq = cell.value("ssRsrq", 0); info.ssSinr = cell.value("ssSinr", 0);
                                info.rsrp = info.ssRsrp; info.signalStrength = info.rsrp;
                            }
                            string cellKey = info.getCellKey();
                            if (info.pci != -1) cellKey += "_PCI" + to_string(info.pci);
                            if (info.rsrp != -140 || info.rssi != 0 || info.dbm != -140 || !info.mcc.empty()) {
                                shared_data->signalHistory.addPoint(ts, info.signalStrength, info.rssi, info.sinr, info.type, cellKey, info.pci);
                                shared_data->pciSignalHistory.addPoint(info.pci, ts, info.signalStrength, info.rssi, info.sinr, info.type, cellKey);
                            }
                            shared_data->cells.push_back(info);
                        }
                        shared_data->telephonySummary = to_string(shared_data->cells.size()) + " вышка(и)";
                    }
                    if (j.contains("location") && !j["location"].is_null()) {
                        auto loc = j["location"];
                        shared_data->lat = loc.value("latitude", 0.0); shared_data->lon = loc.value("longitude", 0.0);
                        shared_data->alt = loc.value("altitude", 0.0); shared_data->accuracy = loc.value("accuracy", 0.0f);
                        shared_data->timestamp = loc.value("time", ts);
                        if (shared_data->map.centerLat == 0.0 && shared_data->map.centerLon == 0.0) {
                            shared_data->map.centerLat = shared_data->lat; shared_data->map.centerLon = shared_data->lon;
                        }
                    }
                    if (msgType == "traffic_stats") {
                        shared_data->netSent = j.value("total_tx_bytes", 0LL); shared_data->netRecv = j.value("total_rx_bytes", 0LL);
                        shared_data->netSent += j.value("mobile_tx_bytes", 0LL); shared_data->netRecv += j.value("mobile_rx_bytes", 0LL);
                    }
                    if (msgType == "combined" && j.contains("networkUsage") && !j["networkUsage"].is_null() && shared_data->filterNetwork) {
                        auto net = j["networkUsage"];
                        shared_data->netSent = net.value("totalBytesSent", 0LL); shared_data->netRecv = net.value("totalBytesReceived", 0LL);
                        if (net.contains("topApps") && net["topApps"].is_array()) {
                            shared_data->topApps.clear();
                            for (const auto& app : net["topApps"]) shared_data->topApps.push_back({app.value("packageName", "unknown"), app.value("bytes", 0LL)});
                        }
                    }
                    if (msgType == "combined" && j.contains("telephony") && j["telephony"].is_array() && shared_data->filterTelephony) {
                        shared_data->cells.clear();
                        for (auto& item : j["telephony"]) {
                            CellInfo info;
                            info.type = item.value("type", "Unknown"); info.isRegistered = item.value("isRegistered", false);
                            if (item.contains("CellIdentityLte")) {
                                auto cid = item["CellIdentityLte"];
                                info.mcc = get_string_safe(cid, "MCC"); info.mnc = get_string_safe(cid, "MNC"); info.cellId = get_string_safe(cid, "CellIdentity");
                                info.lac = get_string_safe(cid, "LAC"); info.tac = get_string_safe(cid, "TAC");
                                info.pci = get_int_safe(cid, "PCI", -1); info.earfcn = get_int_safe(cid, "EARFCN", -1); info.band = get_int_safe(cid, "Band", -1);
                            } else if (item.contains("CellIdentityGSM")) {
                                auto cid = item["CellIdentityGSM"];
                                info.mcc = get_string_safe(cid, "MCC"); info.mnc = get_string_safe(cid, "MNC"); info.cellId = get_string_safe(cid, "CellIdentity");
                                info.lac = get_string_safe(cid, "LAC"); info.bsic = get_int_safe(cid, "BSIC", -1); info.arfcn = get_int_safe(cid, "ARFCN", -1); info.psc = get_int_safe(cid, "PSC", -1);
                            } else if (item.contains("CellIdentityNr") || info.type == "5G_NR") {
                                auto cid = item.contains("CellIdentityNr") ? item["CellIdentityNr"] : item;
                                info.mcc = get_string_safe(cid, "MCC"); info.mnc = get_string_safe(cid, "MNC"); info.nci = get_string_safe(cid, "NCI");
                                if (info.nci == " ") info.nci = get_string_safe(item, "nci"); info.tac = get_string_safe(cid, "TAC");
                                info.pci = get_int_safe(cid, "PCI", -1); if (info.pci == -1) info.pci = get_int_safe(item, "pci", -1);
                                info.nrarfcn = get_int_safe(cid, "Nrarfcn", -1); if (info.nrarfcn == -1) info.nrarfcn = get_int_safe(item, "nrarfcn", -1);
                                info.band = get_int_safe(cid, "Band", -1);
                            } else {
                                info.mcc = get_string_safe(item, "mcc"); info.mnc = get_string_safe(item, "mnc"); info.cellId = get_string_safe(item, "cellIdentity");
                                if (info.cellId == " ") info.cellId = get_string_safe(item, "nci"); info.lac = get_string_safe(item, "lac"); info.tac = get_string_safe(item, "tac");
                                info.pci = get_int_safe(item, "pci", -1); info.earfcn = get_int_safe(item, "earfcn", -1);
                                if (info.earfcn == -1) info.earfcn = get_int_safe(item, "nrarfcn", -1);
                            }
                            if (item.contains("CellSignalStrengthLte")) {
                                auto ss = item["CellSignalStrengthLte"];
                                info.rsrp = get_int_safe(ss, "RSRP", -140); info.rssi = get_int_safe(ss, "RSSI", 0); info.rssnr = get_int_safe(ss, "RSSNR", 0);
                                info.sinr = info.rssnr; info.rsrq = get_int_safe(ss, "RSRQ", 0);
                                info.asuLevel = get_int_safe(ss, "ASU", 0); if (info.asuLevel == 0) info.asuLevel = get_int_safe(ss, "asuLevel", 0);
                                info.cqi = get_int_safe(ss, "CQI", 0); if (info.cqi == 0) info.cqi = get_int_safe(ss, "cqi", 0);
                                info.timingAdvance = get_int_safe(ss, "TimingAdvance", 0); if (info.timingAdvance == 0) info.timingAdvance = get_int_safe(ss, "timingAdvance", 0);
                                info.dbm = get_int_safe(ss, "Dbm", -140); info.signalStrength = info.rsrp;
                            } else if (item.contains("CellSignalStrengthGsm")) {
                                auto ss = item["CellSignalStrengthGsm"];
                                info.dbm = get_int_safe(ss, "Dbm", -140); info.rssi = info.dbm; info.signalStrength = info.dbm;
                                info.timingAdvance = get_int_safe(ss, "TimingAdvance", 0); if (info.timingAdvance == 0) info.timingAdvance = get_int_safe(ss, "timingAdvance", 0);
                                info.asuLevel = get_int_safe(ss, "ASU", 0); if (info.asuLevel == 0) info.asuLevel = get_int_safe(ss, "asuLevel", 0);
                            } else if (item.contains("CellSignalStrengthNr") || info.type == "5G_NR") {
                                auto ss = item.contains("CellSignalStrengthNr") ? item["CellSignalStrengthNr"] : item;
                                info.ssRsrp = get_int_safe(ss, "SS-RSRP", -140); if (info.ssRsrp == -140) info.ssRsrp = get_int_safe(item, "ssRsrp", -140);
                                info.rsrp = info.ssRsrp; info.signalStrength = info.rsrp;
                                info.ssRsrq = get_int_safe(ss, "SS-RSRQ", 0); if (info.ssRsrq == 0) info.ssRsrq = get_int_safe(item, "ssRsrq", 0); info.rsrq = info.ssRsrq;
                                info.ssSinr = get_int_safe(ss, "SS-SINR", 0); if (info.ssSinr == 0) info.ssSinr = get_int_safe(item, "ssSinr", 0); info.sinr = info.ssSinr;
                                info.timingAdvance = get_int_safe(ss, "TimingAdvance", 0); if (info.timingAdvance == 0) info.timingAdvance = get_int_safe(item, "timingAdvance", 0);
                                info.asuLevel = get_int_safe(ss, "ASU", 0); if (info.asuLevel == 0) info.asuLevel = get_int_safe(item, "asuLevel", 0);
                            } else {
                                info.rsrp = get_int_safe(item, "rsrp", -140); if (info.rsrp == -140) info.rsrp = get_int_safe(item, "ssRsrp", -140); info.signalStrength = info.rsrp;
                                info.rssi = get_int_safe(item, "rssi", 0); if (info.rssi == 0) info.rssi = get_int_safe(item, "dbm", 0); info.dbm = info.rssi;
                                info.sinr = get_int_safe(item, "sinr", 0); if (info.sinr == 0) info.sinr = get_int_safe(item, "ssSinr", 0);
                                info.rsrq = get_int_safe(item, "rsrq", 0); if (info.rsrq == 0) info.rsrq = get_int_safe(item, "ssRsrq", 0);
                                info.asuLevel = get_int_safe(item, "asuLevel", 0); info.cqi = get_int_safe(item, "cqi", 0); info.timingAdvance = get_int_safe(item, "timingAdvance", 0);
                            }
                            string cellKey = info.getCellKey();
                            string cellKeyWithPci = cellKey;
                            if (info.pci != -1) cellKeyWithPci += "_PCI" + to_string(info.pci);
                            shared_data->cells.push_back(info);
                            shared_data->signalHistory.addPoint(ts, info.signalStrength, info.rssi, info.sinr, info.type, cellKeyWithPci, info.pci);
                            shared_data->pciSignalHistory.addPoint(info.pci, ts, info.signalStrength, info.rssi, info.sinr, info.type, cellKeyWithPci);
                        }
                        shared_data->telephonySummary = to_string(shared_data->cells.size()) + " вышка(и)";
                    }
                }
                if (shared_data->filterLocation && (shared_data->lat != 0 || shared_data->lon != 0))
                    shared_data->locationHistory.addPoint(shared_data->lat, shared_data->lon, shared_data->alt, shared_data->accuracy, shared_data->timestamp);
                if (db->isConnected()) {
                    dbQueue->enqueue([db, shared_data, ts]() {
                        if (shared_data->filterLocation) db->insertLocation(shared_data->lat, shared_data->lon, shared_data->alt, shared_data->accuracy, shared_data->timestamp);
                        if (shared_data->filterTelephony) for (const auto& cell : shared_data->cells) { db->insertTelephony(cell, ts); shared_data->dbInsertCount.fetch_add(1); }
                        if (shared_data->filterNetwork && !shared_data->topApps.empty()) db->insertNetworkUsage(shared_data->netSent, shared_data->netRecv, ts, shared_data->topApps);
                    });
                    shared_data->dbPendingTasks.store(dbQueue->pendingTasks());
                }
                auto nowTime = chrono::steady_clock::now();
                if (chrono::duration_cast<chrono::seconds>(nowTime - lastStats).count() >= 1) {
                    shared_data->messagesPerSecond.store(msgPerSec);
                    cout << "[СТАТИСТИКА] " << msgPerSec << " сообщ/сек | Очередь: " << dbQueue->pendingTasks() << endl;
                    msgPerSec = 0; lastStats = nowTime;
                }
            }
        } catch (...) {
            if (!should_stop->load()) this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
}