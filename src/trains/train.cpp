// ============================================================================
// Member 1: Train Processes & Scheduling Subsystem
// ============================================================================
#include "train.h"
#include <chrono>

namespace rail {

using namespace std;

Train::Train(TrainId id, int priority, vector<ResourceId> route,
             ResourceManager& rm, Controller& ctl, SignalControl* signals, int occupy_ms)
    : id_(id), priority_(priority), route_(move(route)),
      occupy_ms_(occupy_ms), rm_(rm), ctl_(ctl), signals_(signals) {}

void Train::start() { 
    thread_ = thread(&Train::run, this); 
}

void Train::join() { 
    if (thread_.joinable()) {
        thread_.join(); 
    }
}

void Train::setState(TrainState s) {
    {
        lock_guard<mutex> lk(pcb_mtx_);
        state_ = s;
    }
    const char* names[] = {"NEW", "READY", "RUNNING", "WAITING", "TERMINATED"};
    ctl_.submitEvent({"STATE", id_, current_, names[static_cast<int>(s)]});
}

TrainView Train::view() {
    lock_guard<mutex> lk(pcb_mtx_);
    ResourceId next = (step_ < route_.size()) ? route_[step_] : -1;
    return TrainView{id_, state_, priority_, current_, next};
}

void Train::run() {
    setState(TrainState::READY);
    for (step_ = 0; step_ < route_.size(); ++step_) {
        const ResourceId res = route_[step_];

        if (signals_ && signals_->getState(res) == SignalState::RED) {
            setState(TrainState::WAITING);
            signals_->hold(id_);
        }

        setState(TrainState::WAITING);
        rm_.request(id_, res);

        {
            lock_guard<mutex> lk(pcb_mtx_);
            current_ = res;
        }
        setState(TrainState::RUNNING);
        this_thread::sleep_for(chrono::milliseconds(occupy_ms_));

        rm_.release(id_, res);
        {
            lock_guard<mutex> lk(pcb_mtx_);
            current_ = -1;
        }
    }
    setState(TrainState::TERMINATED);
}

}  // namespace rail
