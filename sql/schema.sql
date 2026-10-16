-- Member 5: Simulation Sessions & IPC Commands


CREATE TABLE IF NOT EXISTS simulation_runs (
    run_id       SERIAL PRIMARY KEY,
    started_at   TIMESTAMP DEFAULT now(),
    ended_at     TIMESTAMP,
    status       VARCHAR(20) DEFAULT 'RUNNING',
    config_notes TEXT
);

CREATE TABLE IF NOT EXISTS event_log (
    id           SERIAL PRIMARY KEY,
    run_id       INTEGER,
    ts           TIMESTAMP DEFAULT now(),
    type         VARCHAR(50),
    train_id     INTEGER,
    resource_id  INTEGER,
    message      TEXT
);

CREATE TABLE IF NOT EXISTS ipc_commands_log (
    id           SERIAL PRIMARY KEY,
    run_id       INTEGER,
    cmd          VARCHAR(50),
    source       VARCHAR(50) DEFAULT 'IPC_PIPE',
    received_at  TIMESTAMP DEFAULT now()
);

-- Member 1: Train Processes, Routes & Scheduling Metrics

CREATE TABLE IF NOT EXISTS trains (
    train_id     INTEGER PRIMARY KEY,
    name         VARCHAR(50),
    priority     INTEGER DEFAULT 1,
    created_at   TIMESTAMP DEFAULT now()
);

CREATE TABLE IF NOT EXISTS train_routes (
    route_id     SERIAL PRIMARY KEY,
    train_id     INTEGER,
    step_order   INTEGER,
    resource_id  INTEGER,
    occupy_ms    INTEGER DEFAULT 500
);

CREATE TABLE IF NOT EXISTS train_scheduling_metrics (
    id               SERIAL PRIMARY KEY,
    run_id           INTEGER,
    train_id         INTEGER,
    turnaround_ms    INTEGER,
    total_waiting_ms INTEGER,
    burst_count      INTEGER
);


-- Member 2: Resources, Allocations & Contention

CREATE TABLE IF NOT EXISTS resources (
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) DEFAULT '',
    type         VARCHAR(20) DEFAULT 'exclusive',
    capacity     INTEGER DEFAULT 1
);

CREATE TABLE IF NOT EXISTS allocations (
    id           SERIAL PRIMARY KEY,
    run_id       INTEGER,
    train_id     INTEGER,
    resource_id  INTEGER,
    acquired_at  TIMESTAMP DEFAULT now(),
    released_at  TIMESTAMP,
    duration_ms  INTEGER
);

CREATE TABLE IF NOT EXISTS waiting_requests (
    id           SERIAL PRIMARY KEY,
    run_id       INTEGER,
    train_id     INTEGER,
    resource_id  INTEGER,
    requested_at TIMESTAMP DEFAULT now(),
    outcome      VARCHAR(20),
    wait_ms      INTEGER
);


-- Member 3: Deadlock Incidents & Collision Avoidance

CREATE TABLE IF NOT EXISTS deadlock_incidents (
    incident_id      SERIAL PRIMARY KEY,
    run_id           INTEGER,
    detected_at      TIMESTAMP DEFAULT now(),
    victim_train_id  INTEGER,
    cycle_length     INTEGER,
    resolution       VARCHAR(50)
);

CREATE TABLE IF NOT EXISTS deadlock_edges (
    id               SERIAL PRIMARY KEY,
    incident_id      INTEGER,
    from_train       INTEGER,
    to_train         INTEGER,
    resource_id      INTEGER
);

CREATE TABLE IF NOT EXISTS collision_avoidance_logs (
    id               SERIAL PRIMARY KEY,
    run_id           INTEGER,
    ts               TIMESTAMP DEFAULT now(),
    train_id         INTEGER,
    resource_id      INTEGER,
    conflict_train   INTEGER,
    action_taken     VARCHAR(50)
);

-- Member 4: Signals, State History & Safety Incidents

CREATE TABLE IF NOT EXISTS signals (
    signal_id        INTEGER PRIMARY KEY,
    resource_id      INTEGER,
    current_state    VARCHAR(10) DEFAULT 'RED'
);

CREATE TABLE IF NOT EXISTS signal_state_history (
    id               SERIAL PRIMARY KEY,
    signal_id        INTEGER,
    old_state        VARCHAR(10),
    new_state        VARCHAR(10),
    changed_at       TIMESTAMP DEFAULT now(),
    cause            VARCHAR(50)
);

CREATE TABLE IF NOT EXISTS safety_incidents (
    id               SERIAL PRIMARY KEY,
    run_id           INTEGER,
    ts               TIMESTAMP DEFAULT now(),
    severity         VARCHAR(20),
    train_id         INTEGER,
    signal_id        INTEGER,
    message          TEXT
);

-- Member 5: Indexes & Query Optimization

CREATE INDEX IF NOT EXISTS idx_event_run_ts ON event_log(run_id, ts);
CREATE INDEX IF NOT EXISTS idx_allocations_run_res ON allocations(run_id, resource_id);
CREATE INDEX IF NOT EXISTS idx_waiting_run_train ON waiting_requests(run_id, train_id);
