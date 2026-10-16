// ============================================================================
// Member 5: IPC & Simulation Controller
// ============================================================================
#include "controller.h"
#include <iostream>

namespace rail {

using namespace std;

void ControllerImpl::start() { 
    thread_ = thread(&ControllerImpl::dispatch, this); 
}

void ControllerImpl::stop() {
    {
        lock_guard<mutex> lk(mtx_);
        if (stopping_) return;
        stopping_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void ControllerImpl::submitEvent(const Event& e) {
    {
        lock_guard<mutex> lk(mtx_);
        queue_.push_back(e);
    }
    cv_.notify_one();
}

void ControllerImpl::setState(SimState s) { 
    sim_.store(s); 
}

void ControllerImpl::step() {}

void ControllerImpl::dispatch() {
    unique_lock<mutex> lk(mtx_);
    for (;;) {
        cv_.wait(lk, [&] { return stopping_ || !queue_.empty(); });
        while (!queue_.empty()) {
            Event e = move(queue_.front());
            queue_.pop_front();
            lk.unlock();

            cout << "[event] " << e.type << " train=" << e.train
                 << " res=" << e.resource
                 << (e.message.empty() ? "" : " " + e.message) << "\n";
            persist_.enqueueRecord({Record::Kind::EVENT, e.type, e.train, e.resource,
                                    e.message, 0, 0});

            lk.lock();
        }
        if (stopping_) break;
    }
}

}  // namespace rail
