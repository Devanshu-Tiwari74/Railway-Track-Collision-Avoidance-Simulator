// ============================================================================
// Member 2: Resource Management & Concurrency Primitives
// ============================================================================
#include "resource_manager.h"
#include <algorithm>
#include <chrono>

namespace rail {

using namespace std;

static long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void ResourceManagerImpl::addResource(ResourceId id, ResourceType type, int capacity, const string& name) {
    lock_guard<mutex> lk(mtx_);
    table_[id] = Res{name, type, capacity, {}, {}};
}

bool ResourceManagerImpl::grantable(const Res& r, TrainId t) const {
    return !r.waiters.empty() && r.waiters.front() == t &&
           static_cast<int>(r.holders.size()) < r.capacity;
}

bool ResourceManagerImpl::request(TrainId train, ResourceId resource) {
    if (cd_) {
        cd_->checkSafe(train, resource);
    }

    unique_lock<mutex> lk(mtx_);
    Res& r = table_.at(resource);
    r.waiters.push_back(train);

    const long long t0 = now_ms();
    const bool had_to_wait = !grantable(r, train);
    cv_.wait(lk, [&] { return grantable(r, train); });

    r.waiters.pop_front();
    r.holders.push_back(train);
    const long long t1 = now_ms();

    if (had_to_wait) {
        persist_.enqueueRecord({Record::Kind::WAITING, "granted", train, resource, "", t0, t1});
    }
    persist_.enqueueRecord({Record::Kind::ALLOCATION, "acquire", train, resource, "", t1, 0});
    persist_.enqueueRecord({Record::Kind::EVENT, "ACQUIRE", train, resource, "", 0, 0});
    return true;
}

bool ResourceManagerImpl::tryRequest(TrainId train, ResourceId resource) {
    if (cd_ && !cd_->checkSafe(train, resource)) {
        return false;
    }

    unique_lock<mutex> lk(mtx_);
    Res& r = table_.at(resource);
    if (!r.waiters.empty() || static_cast<int>(r.holders.size()) >= r.capacity) {
        persist_.enqueueRecord({Record::Kind::WAITING, "denied", train, resource, "", 0, 0});
        return false;
    }
    r.holders.push_back(train);
    persist_.enqueueRecord({Record::Kind::ALLOCATION, "acquire", train, resource, "", now_ms(), 0});
    persist_.enqueueRecord({Record::Kind::EVENT, "ACQUIRE", train, resource, "", 0, 0});
    return true;
}

void ResourceManagerImpl::release(TrainId train, ResourceId resource) {
    {
        lock_guard<mutex> lk(mtx_);
        Res& r = table_.at(resource);
        auto it = find(r.holders.begin(), r.holders.end(), train);
        if (it != r.holders.end()) {
            r.holders.erase(it);
        }
        persist_.enqueueRecord({Record::Kind::EVENT, "RELEASE", train, resource, "", 0, 0});
    }
    cv_.notify_all();
}

Snapshot ResourceManagerImpl::snapshot() {
    lock_guard<mutex> lk(mtx_);
    Snapshot s;
    for (const auto& [id, r] : table_) {
        for (TrainId h : r.holders) {
            s.allocations.push_back({h, id});
        }
        for (TrainId w : r.waiters) {
            s.waits.push_back({w, id});
        }
    }
    return s;
}

}  // namespace rail
