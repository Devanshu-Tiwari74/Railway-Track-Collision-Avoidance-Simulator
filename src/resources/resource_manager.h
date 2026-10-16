// ============================================================================
// Member 2: Resource Management & Concurrency Primitives
// ============================================================================
#pragma once

#include "interfaces.h"
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace rail {

using namespace std;

class ResourceManagerImpl : public ResourceManager {
public:
    explicit ResourceManagerImpl(Persistence& persist) : persist_(persist) {}

    void setCollisionEngine(CollisionDeadlock* cd) { cd_ = cd; }
    void addResource(ResourceId id, ResourceType type, int capacity, const string& name = "");

    bool     request(TrainId train, ResourceId resource) override;
    bool     tryRequest(TrainId train, ResourceId resource) override;
    void     release(TrainId train, ResourceId resource) override;
    Snapshot snapshot() override;

private:
    struct Res {
        string          name;
        ResourceType    type     = ResourceType::EXCLUSIVE;
        int             capacity = 1;
        vector<TrainId> holders;
        deque<TrainId>  waiters;
    };

    bool grantable(const Res& r, TrainId t) const;

    Persistence&         persist_;
    CollisionDeadlock*   cd_ = nullptr;
    map<ResourceId, Res> table_;
    mutex                mtx_;
    condition_variable   cv_;
};

}  // namespace rail
