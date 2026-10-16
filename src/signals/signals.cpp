// ============================================================================
// Member 4: Signal Control & Safety Incident Management
// ============================================================================
#include "signals.h"

namespace rail {

using namespace std;

static const char* signal_state_str(SignalState s) {
    switch (s) {
    case SignalState::RED:    return "RED";
    case SignalState::YELLOW: return "YELLOW";
    case SignalState::GREEN:  return "GREEN";
    }
    return "UNKNOWN";
}

SignalManager::SignalManager(Persistence& persist, Controller& controller)
    : persist_(persist), controller_(controller) {}

void SignalManager::addSignal(ResourceId resource, SignalState initial) {
    lock_guard<mutex> lk(mtx_);
    signals_[resource] = initial;
}

SignalState SignalManager::getState(ResourceId resource) {
    lock_guard<mutex> lk(mtx_);
    auto it = signals_.find(resource);
    return (it != signals_.end()) ? it->second : SignalState::GREEN;
}

void SignalManager::setState(ResourceId resource, SignalState state) {
    {
        lock_guard<mutex> lk(mtx_);
        signals_[resource] = state;
    }
    const char* str = signal_state_str(state);
    controller_.submitEvent({"SIGNAL", -1, resource, str});
    persist_.enqueueRecord({Record::Kind::SIGNAL, str, -1, resource, ""});
}

void SignalManager::hold(TrainId train) {
    unique_lock<mutex> lk(mtx_);
    held_trains_[train] = true;
    controller_.submitEvent({"HOLD", train, -1, "Train held at signal"});
    cv_.wait(lk, [&] { return !held_trains_[train]; });
}

void SignalManager::clear(TrainId train) {
    {
        lock_guard<mutex> lk(mtx_);
        held_trains_[train] = false;
    }
    controller_.submitEvent({"CLEAR", train, -1, "Train cleared signal"});
    cv_.notify_all();
}

void SignalManager::reportIncident(const string& description) {
    {
        lock_guard<mutex> lk(mtx_);
        for (auto& [res, state] : signals_) {
            state = SignalState::RED;
        }
    }
    controller_.submitEvent({"INCIDENT", -1, -1, description});
    persist_.enqueueRecord({Record::Kind::SAFETY, "HAZARD_ALERT", -1, -1, description});
}

}  // namespace rail
