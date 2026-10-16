// Member 3: Collision Detection & Deadlock Engine

#include "collision.h"
#include <string>

namespace rail {

using namespace std;

CollisionEngine::CollisionEngine(ResourceManager& rm, SignalControl& signals, 
                                 Persistence& persist, Controller& controller)
    : rm_(rm), signals_(signals), persist_(persist), controller_(controller) {}

bool CollisionEngine::checkSafe(TrainId train, ResourceId resource) {
    Snapshot snap = rm_.snapshot();

    for (const auto& alloc : snap.allocations) {
        if (alloc.resource == resource && alloc.train != train) {
            lock_guard<mutex> lk(mtx_);
            signals_.setState(resource, SignalState::RED);
            
            string msg = "Collision averted: resource held by train " + to_string(alloc.train);
            controller_.submitEvent({"COLLISION_AVERTED", train, resource, msg});
            persist_.enqueueRecord({Record::Kind::EVENT, "COLLISION_AVERTED", train, resource, msg});
            return false;
        }
    }

    signals_.setState(resource, SignalState::GREEN);
    return true;
}

vector<DeadlockCycle> CollisionEngine::detectDeadlock() {
    return {};
}

void CollisionEngine::recover(const DeadlockCycle& cycle) {
    (void)cycle;
}

}  