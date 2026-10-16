// interfaces.h — Unified cross-module contracts and shared types
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace rail {

using TrainId    = int;
using ResourceId = int;

// ============================================================================
// Member 1: Train Processes & Scheduling Subsystem
// ============================================================================

enum class TrainState { 
    NEW, 
    READY, 
    RUNNING, 
    WAITING, 
    TERMINATED 
};

struct TrainView {
    TrainId    id       = -1;
    TrainState state    = TrainState::NEW;
    int        priority = 0;
    ResourceId current  = -1;
    ResourceId next     = -1;
};

struct Scheduler {
    virtual ~Scheduler() = default;
    virtual void scheduleTurn() = 0;
    virtual void registerTrain(TrainId train, int priority) = 0;
    virtual void updateState(TrainId train, TrainState state) = 0;
};

// ============================================================================
// Member 2: Resource Management & Concurrency Primitives
// ============================================================================

enum class ResourceType { 
    EXCLUSIVE, 
    COUNTING 
};

struct Allocation { 
    TrainId    train; 
    ResourceId resource; 
};

struct WaitEdge { 
    TrainId    train; 
    ResourceId resource; 
};

struct Snapshot {
    std::vector<Allocation> allocations;
    std::vector<WaitEdge>   waits;
};

struct ResourceManager {
    virtual ~ResourceManager() = default;
    virtual bool     request(TrainId train, ResourceId resource) = 0;
    virtual bool     tryRequest(TrainId train, ResourceId resource) = 0;
    virtual void     release(TrainId train, ResourceId resource) = 0;
    virtual Snapshot snapshot() = 0;
};

struct Record {
    enum class Kind { 
        EVENT, 
        ALLOCATION, 
        WAITING, 
        DEADLOCK, 
        SIGNAL, 
        SAFETY, 
        METRIC 
    } kind = Kind::EVENT;

    std::string type;
    TrainId     train      = -1;
    ResourceId  resource   = -1;
    std::string message;
    long long   t_start_ms = 0;
    long long   t_end_ms   = 0;
};

struct Persistence {
    virtual ~Persistence() = default;
    virtual void enqueueRecord(const Record& record) = 0;
};

// ============================================================================
// Member 3: Collision Detection & Deadlock Engine
// ============================================================================

struct DeadlockCycle {
    std::vector<TrainId>    trains;
    std::vector<ResourceId> resources;
    TrainId                 victim = -1;
};

struct CollisionDeadlock {
    virtual ~CollisionDeadlock() = default;
    virtual bool                       checkSafe(TrainId train, ResourceId resource) = 0;
    virtual std::vector<DeadlockCycle> detectDeadlock() = 0;
    virtual void                       recover(const DeadlockCycle& cycle) = 0;
};

// ============================================================================
// Member 4: Signal Control & Safety Incident Management
// ============================================================================

enum class SignalState { 
    RED, 
    YELLOW, 
    GREEN 
};

struct SignalControl {
    virtual ~SignalControl() = default;
    virtual SignalState getState(ResourceId resource) = 0;
    virtual void        setState(ResourceId resource, SignalState state) = 0;
    virtual void        hold(TrainId train) = 0;
    virtual void        clear(TrainId train) = 0;
    virtual void        reportIncident(const std::string& description) = 0;
};

// ============================================================================
// Member 5: IPC & Simulation Controller
// ============================================================================

enum class SimState { 
    RUNNING, 
    PAUSED, 
    STOPPED 
};

struct Event {
    std::string type;
    TrainId     train    = -1;
    ResourceId  resource = -1;
    std::string message;
};

struct Controller {
    virtual ~Controller() = default;
    virtual void submitEvent(const Event& event) = 0;
    virtual void step() = 0;
    virtual void setState(SimState state) = 0;
};

}  // namespace rail
