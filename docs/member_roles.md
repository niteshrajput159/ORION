# OScope: Engineering Team Roles & Component Ownership

The OScope architecture is organized into four distinct subsystem domains, integrating operating systems engineering with database management and full-stack software development.

---

## Member 1: Process & CPU Observation Specialist

### OS Layer Responsibilities
- **Kernel Interfacing**: Implemented `/proc/stat` and `/proc/[pid]/stat` low-level parsing engines (`core/src/monitoring/cpu_calculator.cpp`, `core/src/monitoring/process_monitor.cpp`).
- **Jiffies Calculation**: Engineered tick differential algorithms accounting for multi-core topology and dynamically shifting wall-clock deltas.
- **Fault-Injection Workloads**: Authored `test_workloads/cpu_hog.cpp` and `test_workloads/thread_hog.cpp`.

### DBMS Layer Responsibilities
- **Entity Architecture**: Designed and implemented the `PROCESS` and `PROCESS_SNAPSHOT` relational tables (`database/schema/schema.sql`).
- **Index Optimization**: Built composite B-Tree indexes on `(pid, timestamp DESC)` for fast process timeline rendering.
- **Analytics Queries**: Authored high-throughput queries in `database/queries/analytics_queries.sql` calculating process resource drift.

---

## Member 2: Memory & Resource Accounting Specialist

### OS Layer Responsibilities
- **Kernel Interfacing**: Implemented `/proc/meminfo` and `/proc/[pid]/statm` parsing engines (`core/src/monitoring/memory_monitor.cpp`, `core/src/monitoring/resource_monitor.cpp`).
- **Descriptor & Page Accounting**: Built `/proc/[pid]/fd` directory scanners and page-size memory calculators (`sysconf(_SC_PAGESIZE)`).
- **Fault-Injection Workloads**: Authored `test_workloads/memory_hog.cpp`, `test_workloads/io_hog.cpp`, and `test_workloads/fd_hog.cpp`.

### DBMS Layer Responsibilities
- **Entity Architecture**: Designed and implemented the `RESOURCE_EVENT` table for cluster/node-level pressure tracking.
- **Time-Series Partitioning**: Structured telemetry retention policies to prevent disk bloating from high-frequency snapshot logging.
- **Analytics Queries**: Formulated aggregations tracking system-wide memory exhaustion trends and swap thrashing.

---

## Member 3: Fault Detection & Deadlock Graph Specialist

### OS Layer Responsibilities
- **Algorithmic Detection**: Implemented the Threshold, Persistence Window, and Hybrid Statistical anomaly detectors (`core/src/detection/`).
- **Wait-For Graph (WFG)**: Constructed in-memory directed graphs of thread mutex locks (`core/src/deadlock/wait_for_graph.cpp`).
- **Cycle Detection**: Implemented 3-color Depth First Search (DFS) for $O(V+E)$ deadlock discovery (`core/src/deadlock/cycle_detector.cpp`).
- **Victim Selection**: Formulated the `LEAST_COST`, `LOWEST_PRIORITY`, and `HIGHEST_RESOURCE` cycle-breaking heuristics (`core/src/deadlock/victim_selector.cpp`).
- **Fault-Injection Workloads**: Authored `test_workloads/deadlock_test.cpp`, `test_workloads/crash_test.cpp`, and `test_workloads/hang_test.cpp`.

### DBMS Layer Responsibilities
- **Entity Architecture**: Designed and implemented the `INCIDENT` table and enumerated severity/type constraints.
- **Relational Integrity**: Enforced foreign key cascades linking incidents to active processes while preserving historical incident logs upon process exit.
- **Analytics Queries**: Formulated MTTD (Mean Time to Detect) analytics queries grouped by incident type.

---

## Member 4: Decision, Recovery & Full-Stack Orchestration Specialist

### OS & Backend Responsibilities
- **Recovery Engine**: Engineered the multi-stage escalation ladder (`nice` $\to$ `SIGTERM` $\to$ `SIGKILL` $\to$ `RESTART`) and sliding-window crash-loop circuit breaker (`core/src/recovery/`).
- **Verification Engine**: Implemented post-action telemetry polling to confirm fault remediation (`core/src/verification/`).
- **IPC Protocol**: Designed the Unix Domain Socket protocol connecting C++ Core to Python Backend (`core/src/communication/`).
- **FastAPI Backend**: Built all REST endpoints, Pydantic data schemas, SQLAlchemy ORM mappings, and security whitelisting (`backend/app/`).
- **React Frontend**: Developed the 11-page dashboard, real-time charts, SVG Wait-For Graph visualizer, and workload controls (`frontend/src/`).
- **Fault-Injection Workload**: Authored `test_workloads/crash_loop.cpp`.

### DBMS Layer Responsibilities
- **Entity Architecture**: Designed and implemented `POLICY`, `ACTION_LOG`, and `RECOVERY` tables with transaction audit trails.
- **Audit Logging**: Maintained immutable execution history tracking POSIX signal dispatches, exit codes, and timestamps.
- **Analytics Queries**: Authored MTTR (Mean Time to Recover) and recovery success rate analytics queries.
