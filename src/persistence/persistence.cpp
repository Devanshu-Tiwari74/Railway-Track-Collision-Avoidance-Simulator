// ============================================================================
// Member 2: Persistence Worker & Database Ingestion
// ============================================================================
#include "persistence.h"
#include <iostream>

#ifdef HAVE_LIBPQXX
#include <pqxx/pqxx>
#endif

namespace rail {

using namespace std;

PersistenceWorker::PersistenceWorker(string conn) : conn_(move(conn)) {}

PersistenceWorker::~PersistenceWorker() { stop(); }

void PersistenceWorker::start() {
#ifdef HAVE_LIBPQXX
    try {
        db_ = new pqxx::connection(conn_);
    } catch (const exception& e) {
        cerr << "[persist] DB connect failed, falling back to stdout: " << e.what() << "\n";
        db_ = nullptr;
    }
#endif
    thread_ = thread(&PersistenceWorker::run, this);
}

void PersistenceWorker::stop() {
    {
        lock_guard<mutex> lk(mtx_);
        if (stopping_) return;
        stopping_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
#ifdef HAVE_LIBPQXX
    delete static_cast<pqxx::connection*>(db_);
    db_ = nullptr;
#endif
}

void PersistenceWorker::enqueueRecord(const Record& r) {
    {
        lock_guard<mutex> lk(mtx_);
        queue_.push_back(r);
    }
    cv_.notify_one();
}

void PersistenceWorker::run() {
    unique_lock<mutex> lk(mtx_);
    for (;;) {
        cv_.wait(lk, [&] { return stopping_ || !queue_.empty(); });
        while (!queue_.empty()) {
            Record r = move(queue_.front());
            queue_.pop_front();
            lk.unlock();
            write(r);
            lk.lock();
        }
        if (stopping_) break;
    }
}

void PersistenceWorker::write(const Record& r) {
#ifdef HAVE_LIBPQXX
    if (db_) {
        try {
            pqxx::work tx(*static_cast<pqxx::connection*>(db_));
            switch (r.kind) {
            case Record::Kind::EVENT:
                tx.exec_params(
                    "INSERT INTO event_log(type, train_id, resource_id, message) "
                    "VALUES ($1,$2,$3,$4)",
                    r.type, r.train, r.resource, r.message);
                break;
            case Record::Kind::ALLOCATION:
                tx.exec_params(
                    "INSERT INTO allocations(train_id, resource_id) VALUES ($1,$2)",
                    r.train, r.resource);
                break;
            case Record::Kind::WAITING:
                tx.exec_params(
                    "INSERT INTO waiting_requests(train_id, resource_id, outcome, wait_ms) "
                    "VALUES ($1,$2,$3,$4)",
                    r.train, r.resource, r.type,
                    static_cast<int>(r.t_end_ms - r.t_start_ms));
                break;
            case Record::Kind::SIGNAL:
                tx.exec_params(
                    "INSERT INTO signal_state_history(signal_id, old_state, new_state, cause) "
                    "VALUES ($1,$2,$3,$4)",
                    r.resource, "", r.type, r.message);
                break;
            case Record::Kind::SAFETY:
                tx.exec_params(
                    "INSERT INTO safety_incidents(severity, train_id, signal_id, message) "
                    "VALUES ($1,$2,$3,$4)",
                    r.type, r.train, r.resource, r.message);
                break;
            case Record::Kind::DEADLOCK:
                tx.exec_params(
                    "INSERT INTO deadlock_incidents(victim_train_id, cycle_length, resolution) "
                    "VALUES ($1,$2,$3)",
                    r.train, 2, r.message);
                break;
            case Record::Kind::METRIC:
                tx.exec_params(
                    "INSERT INTO train_scheduling_metrics(train_id, turnaround_ms, total_waiting_ms, burst_count) "
                    "VALUES ($1,$2,$3,$4)",
                    r.train, static_cast<int>(r.t_end_ms - r.t_start_ms), 0, 1);
                break;
            }
            tx.commit();
            return;
        } catch (const exception& e) {
            cerr << "[persist] write failed: " << e.what() << "\n";
        }
    }
#endif
    cout << "[persist] " << r.type << " train=" << r.train
         << " res=" << r.resource << " " << r.message << "\n";
}

}  // namespace rail
