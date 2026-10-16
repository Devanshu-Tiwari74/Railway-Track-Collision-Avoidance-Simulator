#include "collision/collision.h"
#include "controller/controller.h"
#include "persistence/persistence.h"
#include "resources/resource_manager.h"
#include "signals/signals.h"
#include "trains/train.h"

#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>

using namespace std;
using namespace rail;

static unordered_map<string, string> loadEnv(const string& filename = ".env") {
    unordered_map<string, string> env;
    ifstream file(filename);
    if (!file.is_open()) return env;

    string line;
    while (getline(file, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        size_t start = line.find_first_not_of(" \t");
        if (start == string::npos || line[start] == '#') continue;

        size_t eq = line.find('=', start);
        if (eq == string::npos) continue;

        string key = line.substr(start, eq - start);
        string val = line.substr(eq + 1);

        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }

        env[key] = val;
#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
    }
    return env;
}

int main(int argc, char** argv) {
    auto env = loadEnv(".env");
    string host = env.count("DB_HOST") ? env["DB_HOST"] : (env.count("PGHOST") ? env["PGHOST"] : "localhost");
    string port = env.count("DB_PORT") ? env["DB_PORT"] : (env.count("PGPORT") ? env["PGPORT"] : "5432");
    string dbname = env.count("DB_NAME") ? env["DB_NAME"] : (env.count("PGDATABASE") ? env["PGDATABASE"] : "railway");
    string user = env.count("DB_USER") ? env["DB_USER"] : (env.count("PGUSER") ? env["PGUSER"] : "postgres");
    string password = env.count("DB_PASSWORD") ? env["DB_PASSWORD"] : (env.count("PGPASSWORD") ? env["PGPASSWORD"] : "");

    string defaultConn = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user;
    if (!password.empty()) {
        defaultConn += " password=" + password;
    }

    const string conn = (argc > 1) ? argv[1] : defaultConn;

    // Member 2: Persistence worker (DB ingestion)
    PersistenceWorker persist(conn);
    persist.start();

    // Member 5: Simulation Controller and Event Queue
    ControllerImpl controller(persist);
    controller.start();

    // Member 4: Signal Control and Safety Manager
    SignalManager signals(persist, controller);
    signals.addSignal(1, SignalState::GREEN);
    signals.addSignal(2, SignalState::GREEN);
    signals.addSignal(3, SignalState::GREEN);
    signals.addSignal(4, SignalState::GREEN);

    // Member 2: In-Memory Resource Manager
    ResourceManagerImpl rm(persist);
    rm.addResource(1, ResourceType::EXCLUSIVE, 1, "Track 1");
    rm.addResource(2, ResourceType::EXCLUSIVE, 1, "Junction 2");
    rm.addResource(3, ResourceType::EXCLUSIVE, 1, "Track 3");
    rm.addResource(4, ResourceType::COUNTING,  2, "Platform 4 (2 Bays)");

    // Member 3: Collision Avoidance Engine
    CollisionEngine collision(rm, signals, persist, controller);
    rm.setCollisionEngine(&collision);

    // Member 1: Trains and PCB state tracking
    vector<unique_ptr<Train>> trains;
    trains.push_back(make_unique<Train>(1, 1, vector<ResourceId>{1, 2, 4}, rm, controller, &signals, 250));
    trains.push_back(make_unique<Train>(2, 2, vector<ResourceId>{3, 2, 4}, rm, controller, &signals, 250));

    cout << "=== Railway Simulation ===\n";
    for (auto& t : trains) {
        t->start();
    }

    for (auto& t : trains) {
        t->join();
    }

    controller.stop();
    persist.stop();
    cout << "=== Simulation complete ===\n";
    return 0;
}
