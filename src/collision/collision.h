// ============================================================================
// Member 3: Collision Detection & Deadlock Engine
// ============================================================================
#pragma once

#include "interfaces.h"
#include <mutex>
#include <vector>

namespace rail {

using namespace std;

class CollisionEngine : public CollisionDeadlock {
public:
    CollisionEngine(ResourceManager& rm, SignalControl& signals, 
                    Persistence& persist, Controller& controller);

    bool                  checkSafe(TrainId train, ResourceId resource) override;
    vector<DeadlockCycle> detectDeadlock() override;
    void                  recover(const DeadlockCycle& cycle) override;

private:
    ResourceManager& rm_;
    SignalControl&   signals_;
    Persistence&     persist_;
    Controller&      controller_;
    mutex            mtx_;
};

}  // namespace rail
