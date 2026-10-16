// ============================================================================
// Member 2: Persistence Worker & Database Ingestion
// ============================================================================
#pragma once

#include "interfaces.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace rail {

using namespace std;

class PersistenceWorker : public Persistence {
public:
    explicit PersistenceWorker(string conn);
    ~PersistenceWorker() override;

    void enqueueRecord(const Record& r) override;

    void start();
    void stop();

private:
    void run();
    void write(const Record& r);

    string              conn_;
    deque<Record>       queue_;
    mutex               mtx_;
    condition_variable  cv_;
    thread              thread_;
    bool                stopping_ = false;
    void*               db_ = nullptr;
};

}  // namespace rail
