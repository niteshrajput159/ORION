# OScope: Complete Architecture & System Design

OScope (**Intelligent OS Behavior & Recovery**) is a database-driven, autonomous Linux operating system observability, fault detection, and automated remediation platform.

Unlike passive APM tools or monitoring dashboards, OScope is an active **closed-loop control system** governed by the core principle:

$$\text{Observe} \longrightarrow \text{Detect} \longrightarrow \text{Decide} \longrightarrow \text{Act} \longrightarrow \text{Verify} \longrightarrow \text{Recover} \longrightarrow \text{Log}$$

---

## 1. High-Level Architectural Diagram

```
                                      +------------------------------------+
                                      |          REACT FRONTEND UI         |
                                      |   Dashboard, Process Monitor,      |
                                      |   Incidents, WFG Deadlock Viewer   |
                                      +-----------------+------------------+
                                                        |
                                          HTTP / REST   |  (Port :5173 / :80)
                                                        v
                                      +------------------------------------+
                                      |       FASTAPI BACKEND SERVICE      |
                                      |   API Routers, Analytics Engine,   |
                                      |   Policy Whitelist, AI Diagnostics |
                                      +--------+------------------+--------+
                                               |                  |
                               PostgreSQL Pool |                  | Unix Domain Socket IPC
                               (SQLAlchemy)    |                  | (/tmp/oscope.sock)
                                               v                  v
+-----------------------------+       +----------------+  +------------------------------------+
|     POSTGRESQL DATABASE     |       |  TEST CONTROL  |  |         OSCOPE C++ CORE            |
|                             |       |   WORKLOADS    |  |                                    |
|  - PROCESS                  |<------+ (cpu_hog,      |  |  +------------------------------+  |
|  - PROCESS_SNAPSHOT         |       |  memory_hog,   |  |  | Observation & /proc Monitor  |  |
|  - INCIDENT                 |       |  crash_test,   |  |  +--------------+---------------+  |
|  - POLICY                   |       |  deadlock,     |  |                 |                  |
|  - ACTION_LOG               |       |  hang_test)    |  |  +--------------v---------------+  |
|  - RECOVERY                 |       +----------------+  |  | Detection Engine             |  |
|  - RESOURCE_EVENT           |                           |  | (Threshold, Persistence, Z)  |  |
+-----------------------------+                           |  +--------------+---------------+  |
                                                          |                 |                  |
                                                          |  +--------------v---------------+  |
                                                          |  | Decision & Recovery Engine   |  |
                                                          |  | (Escalation Ladder, Backoff) |  |
                                                          |  +--------------+---------------+  |
                                                          |                 |                  |
                                                          |  +--------------v---------------+  |
                                                          |  | Linux Process Controller     |  |
                                                          |  | (kill, setpriority, waitpid) |  |
                                                          |  +--------------+---------------+  |
                                                          |                 |                  |
                                                          |  +--------------v---------------+  |
                                                          |  | Verification Engine          |  |
                                                          |  +------------------------------+  |
                                                          +------------------------------------+
```

---

## 2. Core Execution Loop

### Phase 1: Observation
The C++ Observation module scans the operating system state every 1.0 second:
- **System Metrics**: Reads aggregate jiffies from `/proc/stat` and physical memory from `/proc/meminfo`.
- **Process Metrics**: Iterates over `/proc/[pid]/` entries to extract tick counts (`utime`, `stime`), RSS memory pages (`/proc/[pid]/statm`), thread counts (`num_threads`), open file descriptors (`/proc/[pid]/fd`), and process state (`R`, `S`, `D`, `Z`, `T`).
- **Differential Calculation**: Real CPU percentages are computed by comparing current jiffies with the previous snapshot over the elapsed wall-clock interval.

### Phase 2: Detection
Telemetry is evaluated by three detection engines:
1. **Threshold Detector**: Compares instantaneous values against policy limits (e.g., CPU > 85%, RSS > 500 MB).
2. **Persistence Detector**: Verifies that breaches persist over $N$ consecutive samples (default $N=3$) to suppress transient spikes.
3. **Statistical Anomaly Detector**: Computes rolling Z-Scores ($\mu + 2.5\sigma$) to flag abnormal shifts even below absolute maximums.
4. **Deadlock Detector**: Evaluates thread lock acquisition graphs, running Tarjan's DFS algorithm to discover directed cycles.

### Phase 3: Incident Creation & DBMS Ingestion
Upon confirmation, a formal incident is registered:
- Assigned UUID/ID, Incident Type (`HIGH_CPU`, `HIGH_MEMORY`, `PROCESS_CRASH`, `CRASH_LOOP`, `HUNG_PROCESS`, `DEADLOCK`, etc.), and Severity (`LOW`, `MEDIUM`, `HIGH`, `CRITICAL`).
- Ingested synchronously into the PostgreSQL `INCIDENT` table via transactional insert with foreign key references to the active `PROCESS`.

### Phase 4: Decision Engine
The Decision Engine matches the incident against active policies in the `POLICY` table:
- Evaluates past incident frequency for the given binary.
- Selects an intervention from the strictly whitelisted action ladder:
  - `CHANGE_PRIORITY` (renice $+10$)
  - `TERMINATE_SIGTERM` (graceful exit request)
  - `TERMINATE_SIGKILL` (unconditional preemption)
  - `RESTART` (controlled respawn via `fork`/`execvp`)
  - `RESOLVE_DEADLOCK` (abort selected victim node)

### Phase 5: Action Execution
The C++ Process Controller invokes POSIX system calls:
- Throttles priority via `setpriority(PRIO_PROCESS, pid, new_nice)`.
- Dispatches POSIX signals via `kill(pid, sig)`.
- Starts deadline timer (default 5.0s) for SIGTERM compliance.
- Records action execution attempt in `ACTION_LOG`.

### Phase 6: Verification
The Verification Engine checks whether the remediation resolved the fault:
- **Termination/Kill**: Polls `/proc/[pid]` or `waitpid()` to confirm the process has terminated within 5 seconds.
- **Priority Adjustment**: Verifies that process CPU consumption drops below threshold over the subsequent 3 observation cycles.
- **Deadlock Resolution**: Re-evaluates the Wait-For Graph to verify all cycles have been broken.

### Phase 7: Recovery Logging & Escalation
- If verified: Marks `RECOVERY` as `COMPLETED` (`success = true`), updates `INCIDENT` status to `RESOLVED`.
- If unverified (timeout or persistent fault): Marks `RECOVERY` as `FAILED`, escalates to the next tier in the escalation ladder (e.g., `SIGTERM` $\to$ `SIGKILL`), and generates a high-severity `RESOURCE_EVENT`.

---

## 3. Communication Protocols

### IPC Socket Protocol (Core $\longleftrightarrow$ Backend)
- **Transport**: Unix Domain Socket (`/tmp/oscope.sock`) with SOCK_STREAM semantics.
- **Message Framing**: Newline-delimited JSON payloads (`\n`).
- **Request Format**:
  ```json
  {
    "command": "TRIGGER_ACTION",
    "pid": 4812,
    "action": "TERMINATE_SIGTERM",
    "params": {"timeout_seconds": 5}
  }
  ```
- **Response Format**:
  ```json
  {
    "status": "OK",
    "command": "TRIGGER_ACTION",
    "message": "Signal SIGTERM dispatched to PID 4812",
    "data": {"pid": 4812, "action": "TERMINATE_SIGTERM", "success": true}
  }
  ```

### REST API Protocol (Backend $\longleftrightarrow$ Frontend)
- Standard HTTP/1.1 JSON APIs with structured Pydantic schemas.
- Interactive OpenAPI documentation available at `/docs` and `/redoc`.
