// ============================================================================
// Member 1: Train Processes & Scheduling Subsystem
// ============================================================================
#pragma once

#include "interfaces.h"
#include <mutex>
#include <thread>
#include <vector>

namespace rail {

using namespace std;

class Train {
public:
    Train(TrainId id, int priority, vector<ResourceId> route,
          ResourceManager& rm, Controller& ctl, SignalControl* signals = nullptr, int occupy_ms = 300);

    void start();
    void join();

    TrainView view();

private:
    void run();
    void setState(TrainState s);

    const TrainId        id_;
    const int            priority_;
    const vector<ResourceId> route_;
    const int            occupy_ms_;
    ResourceManager&     rm_;
    Controller&          ctl_;
    SignalControl*       signals_;

    thread               thread_;
    mutex                pcb_mtx_;
    TrainState           state_   = TrainState::NEW;
    size_t               step_    = 0;
    ResourceId           current_ = -1;
};

}  // namespace rail
