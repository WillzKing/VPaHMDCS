#include "net/zmq_receiver.h"
#include "core/json_parser.h"
#include "db/pgsql_minimal.h"
#include <zmq.hpp>
#include <fstream>

static FilterState g_filterState;

FilterState& GetFilterState() { return g_filterState; }

void SendFilterCommand(const std::string& serverIp, int cmdPort, const FilterState& filters) {
    try {
        zmq::context_t ctx(1);
        zmq::socket_t push(ctx, ZMQ_PUSH);
        push.set(zmq::sockopt::sndtimeo, 1000);
        push.connect("tcp://" + serverIp + ":" + std::to_string(cmdPort));
        
        json cmd = {{"type", "control"}, {"gps", filters.gps}, {"lte", filters.lte}, {"nr", filters.nr}};
        push.send(zmq::buffer(cmd.dump()), zmq::send_flags::none);
    } catch (...) {}
}

void RunZmqReceiver(RuntimeState* st, int dataPort, int , bool writeDb) {
    zmq::context_t ctx(1);
    zmq::socket_t pull(ctx, ZMQ_PULL);
    pull.bind("tcp://0.0.0.0:" + std::to_string(dataPort));
    pull.set(zmq::sockopt::rcvtimeo, 100);

    std::ofstream log("build/received_android.json", std::ios::app);
    PgSqlMinimal db;
    if (writeDb) { DbConfig cfg; db.connect(cfg); db.ensureTable(); }

    while (st->running.load()) {
        zmq::message_t msg;
        if (!pull.recv(msg, zmq::recv_flags::none)) continue;

        const std::string raw(static_cast<char*>(msg.data()), msg.size());
        st->bytesRx.fetch_add((long long)raw.size());
        {
            std::lock_guard<std::mutex> lk(st->mtx);
            st->lastRawJson = raw;
            try { ApplyPacket(st, raw, writeDb, db); } catch (...) {}
        }
        st->packets.fetch_add(1);
        log << raw << '\n';
    }
    if (writeDb) db.disconnect();
}
