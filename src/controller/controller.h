// ============================================================================
// Member 5: IPC & Simulation Controller
// ============================================================================
#pragma once

#include "interfaces.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace rail {

using namespace std;

class ControllerImpl : public Controller {
public:
    explicit ControllerImpl(Persistence& persist) : persist_(persist) {}

    void submitEvent(const Event& e) override;
    void step() override;
    void setState(SimState s) override;

    void start();
    void stop();

private:
    void dispatch();

    Persistence&            persist_;
    deque<Event>            queue_;
    mutex                   mtx_;
    condition_variable      cv_;
    thread                  thread_;
    bool                    stopping_ = false;
    atomic<SimState>        sim_{SimState::RUNNING};
};

}  // namespace rail
