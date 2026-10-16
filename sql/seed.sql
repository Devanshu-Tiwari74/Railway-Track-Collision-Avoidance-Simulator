-- Initial baseline data for railway simulation

-- 1. Master Simulation Session (Member 5)
INSERT INTO simulation_runs (run_id, status, config_notes)
VALUES (1, 'COMPLETED', '50% milestone multi-train simulation run')
ON CONFLICT (run_id) DO NOTHING;

-- 2. Master Train Registry (Member 1)
INSERT INTO trains (train_id, name, priority) VALUES
(1, 'Express T101', 1),
(2, 'Freight T102', 2)
ON CONFLICT (train_id) DO UPDATE SET name = EXCLUDED.name, priority = EXCLUDED.priority;

-- 3. Normalized Train Routes (Member 1)
DELETE FROM train_routes WHERE train_id IN (1, 2);
INSERT INTO train_routes (train_id, step_order, resource_id, occupy_ms) VALUES
(1, 1, 1, 250), -- Train 1: Track 1
(1, 2, 2, 250), -- Train 1: Junction 2
(1, 3, 4, 250), -- Train 1: Platform 4
(2, 1, 3, 250), -- Train 2: Track 3
(2, 2, 2, 250), -- Train 2: Junction 2
(2, 3, 4, 250); -- Train 2: Platform 4

-- 4. Railway Resources & Infrastructure (Member 2)
INSERT INTO resources (id, name, type, capacity) VALUES
(1, 'Track 1', 'exclusive', 1),
(2, 'Junction 2', 'exclusive', 1),
(3, 'Track 3', 'exclusive', 1),
(4, 'Platform 4 (2 Bays)', 'counting', 2)
ON CONFLICT (id) DO UPDATE SET name = EXCLUDED.name, type = EXCLUDED.type, capacity = EXCLUDED.capacity;

-- 5. Signal Hardware Registry (Member 4)
INSERT INTO signals (signal_id, resource_id, current_state) VALUES
(1, 1, 'GREEN'),
(2, 2, 'GREEN'),
(3, 3, 'GREEN'),
(4, 4, 'GREEN')
ON CONFLICT (signal_id) DO UPDATE SET current_state = EXCLUDED.current_state;

-- 6. Collision Avoidance Log Entry (Member 3)
INSERT INTO collision_avoidance_logs (run_id, train_id, resource_id, conflict_train, action_taken)
VALUES (1, 2, 2, 1, 'HELD_AT_SIGNAL_RED');
