# Railway Track Collision Avoidance & Simulation System

C++ OS-concepts simulation: trains as threads contending for shared railway resources,
with asynchronous PostgreSQL persistence. See `PRD.md` / `architecture.md` for the full spec.

## Status

- **Phase 0 (foundation):** done — `include/interfaces.h`, `CMakeLists.txt`, `sql/schema.sql`.
- **Phase 1 (vertical slice):** done — one train walks a 3-resource route end-to-end through
  Controller → ResourceManager → Persistence, with full `NEW→READY→WAITING→RUNNING→TERMINATED`
  state transitions. Verified compiling and running (g++ 15, C++20).
- **Phase 2+ (multi-train FCFS, semaphores, collision/deadlock, signals):** not started.

## Build

Requires a C++20 compiler and CMake. PostgreSQL persistence needs libpqxx; without it
the worker logs records to stdout.

```bash
# Install libpqxx (Windows, sanest path):
#   vcpkg install libpqxx
cmake -B build                       # add -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Quick compile without CMake (stdout fallback, no DB):

```bash
g++ -std=c++20 -Iinclude -pthread src/main.cpp src/*/*.cpp -o railway-sim
```

## Database

```bash
createdb railway
psql -d railway -f sql/schema.sql
```

## Run

```bash
./build/railway-sim "dbname=railway user=postgres host=localhost"   # conn string optional
psql -d railway -c "SELECT * FROM event_log ORDER BY id;"           # inspect afterward
```

## Layout

Each `src/<module>/` folder is owned by one team member; shared types live only in
`include/interfaces.h`.

| Folder | Owner | Module |
|---|---|---|
| `src/trains/` | M1 | Train threads, PCB, states, FCFS |
| `src/resources/`, `src/persistence/` | M2 | Resource table, locking, PostgreSQL worker |
| `src/collision/` | M3 | Collision + deadlock (Phase 2) |
| `src/signals/` | M4 | Signals + incidents (Phase 2) |
| `src/controller/` | M5 | Controller, event queue, sim lifecycle |
