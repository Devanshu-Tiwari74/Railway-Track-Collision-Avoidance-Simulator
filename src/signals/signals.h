// ============================================================================
// Member 4: Signal Control & Safety Incident Management
// ============================================================================
#pragma once

#include "interfaces.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

namespace rail {

using namespace std;

class SignalManager : public SignalControl {
public:
    SignalManager(Persistence& persist, Controller& controller);

    void        addSignal(ResourceId resource, SignalState initial = SignalState::GREEN);
    SignalState getState(ResourceId resource) override;
    void        setState(ResourceId resource, SignalState state) override;
    void        hold(TrainId train) override;
    void        clear(TrainId train) override;
    void        reportIncident(const string& description) override;

private:
    Persistence&                 persist_;
    Controller&                  controller_;
    mutex                        mtx_;
    condition_variable           cv_;
    map<ResourceId, SignalState> signals_;
    map<TrainId, bool>           held_trains_;
};

}  // namespace rail
