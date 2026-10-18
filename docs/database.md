# OScope: Database & Storage Engine Architecture

The OScope DBMS layer is engineered using PostgreSQL to deliver high-throughput telemetry ingestion, relational integrity for OS incident lifecycles, and analytical query performance.

---

## 1. Relational Entity Schema

The database schema comprises **7 core tables** linked with foreign keys, index coverage, and cascade deletion semantics.

```
+--------------------+        +-----------------------+
|      PROCESS       |        |   PROCESS_SNAPSHOT    |
+--------------------+        +-----------------------+
| pid (PK)           |<---+---| id (PK)               |
| name               |    |   | pid (FK)              |
| ppid               |    |   | cpu_percent           |
| user_owner         |    |   | memory_rss_mb         |
| state              |    |   | memory_vms_mb         |
| start_time         |    |   | thread_count          |
| executable_path    |    |   | fd_count              |
+--------------------+    |   | timestamp             |
        ^                 |   +-----------------------+
        |                 |
        |                 |   +-----------------------+
        +-----------------+---|       INCIDENT        |
        |                 |   +-----------------------+
        |                 |   | id (PK)               |
        |                 |   | pid (FK)              |
        |                 |   | incident_type         |
        |                 |   | severity              |
        |                 |   | status                |
        |                 |   | detected_at           |
        |                 |   | resolved_at           |
        |                 |   +-----------------------+
        |                 |               ^
        |                 |               |
        |                 |   +-----------+-----------+
        |                 +---|       RECOVERY        |
        |                     +-----------------------+
        |                     | id (PK)               |
        |                     | incident_id (FK)      |
        |                     | pid (FK)              |
        |                     | policy_id (FK)        |
        |                     | action                |
        |                     | attempt_number        |
        |                     | status                |
        |                     | success (BOOLEAN)     |
        |                     | started_at            |
        |                     | completed_at          |
        |                     +-----------------------+
        |                                 ^
        |                                 |
+-------+------------+        +-----------+-----------+
|       POLICY       |        |      ACTION_LOG       |
+--------------------+        +-----------------------+
| id (PK)            |<---+---| id (PK)               |
| incident_type      |    |   | incident_id (FK)      |
| threshold_value    |    |   | recovery_id (FK)      |
| persistence_count  |    |   | pid                   |
| max_attempts       |    |   | action_name           |
| timeout_seconds    |    |   | exit_code             |
| action             |    |   | output_log            |
| enabled (BOOLEAN)  |    |   | executed_at           |
+--------------------+    |   +-----------------------+
                          |
+--------------------+    |
|   RESOURCE_EVENT   |    |
+--------------------+    |
| id (PK)            |    |
| event_type         |    |
| severity           |    |
| message            |    |
| metric_name        |    |
| metric_value       |    |
| timestamp          |    |
+--------------------+----+
```

---

## 2. Table Specifications & Constraints

### 1. `PROCESS`
Tracks registered and active operating system processes.
- **Constraints**: `PRIMARY KEY (pid)`
- **Columns**: `pid INT`, `name VARCHAR(255)`, `ppid INT`, `user_owner VARCHAR(64)`, `state VARCHAR(16)`, `priority INT`, `start_time TIMESTAMP WITH TIME ZONE`, `executable_path TEXT`, `created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP`

### 2. `PROCESS_SNAPSHOT`
High-frequency time-series telemetry per process.
- **Foreign Key**: `FOREIGN KEY (pid) REFERENCES PROCESS(pid) ON DELETE CASCADE`
- **Index**: `CREATE INDEX idx_proc_snap_pid_ts ON PROCESS_SNAPSHOT(pid, timestamp DESC)`

### 3. `INCIDENT`
Anomalies and violations flagged by the detection engine.
- **Foreign Key**: `FOREIGN KEY (pid) REFERENCES PROCESS(pid) ON DELETE CASCADE`
- **Enums / Checks**:
  - `incident_type IN ('HIGH_CPU', 'HIGH_MEMORY', 'PROCESS_CRASH', 'CRASH_LOOP', 'HUNG_PROCESS', 'DEADLOCK', 'HIGH_IO', 'EXCESSIVE_THREADS', 'EXCESSIVE_FILE_DESCRIPTORS')`
  - `severity IN ('LOW', 'MEDIUM', 'HIGH', 'CRITICAL')`
  - `status IN ('DETECTED', 'IN_RECOVERY', 'RESOLVED', 'FAILED', 'ESCALATED')`
- **Index**: `CREATE INDEX idx_incident_status ON INCIDENT(status, detected_at DESC)`

### 4. `POLICY`
Configurable rules governing automated recovery actions.
- **Constraints**: `UNIQUE (incident_type)`
- **Columns**:
  - `threshold_value DOUBLE PRECISION`
  - `persistence_count INT DEFAULT 3`
  - `max_attempts INT DEFAULT 3`
  - `timeout_seconds INT DEFAULT 5`
  - `action VARCHAR(64) CHECK (action IN ('CHANGE_PRIORITY', 'TERMINATE_SIGTERM', 'TERMINATE_SIGKILL', 'RESTART', 'RESOLVE_DEADLOCK', 'LOG_ONLY'))`
  - `enabled BOOLEAN DEFAULT TRUE`

### 5. `RECOVERY`
Execution attempts and post-action verification outcomes.
- **Foreign Keys**:
  - `FOREIGN KEY (incident_id) REFERENCES INCIDENT(id) ON DELETE CASCADE`
  - `FOREIGN KEY (policy_id) REFERENCES POLICY(id) ON DELETE SET NULL`
- **Columns**: `attempt_number INT`, `action VARCHAR(64)`, `status VARCHAR(32)`, `verification_result TEXT`, `success BOOLEAN`, `started_at TIMESTAMP`, `completed_at TIMESTAMP`

### 6. `ACTION_LOG`
Audit trail of POSIX calls dispatched to Linux processes.
- **Columns**: `id SERIAL PRIMARY KEY`, `incident_id INT`, `pid INT`, `action_name VARCHAR(64)`, `exit_code INT`, `output_log TEXT`, `executed_at TIMESTAMP`

### 7. `RESOURCE_EVENT`
Global system-level event timeline (e.g., node-wide RAM pressure, daemon restarts).
- **Columns**: `id SERIAL PRIMARY KEY`, `event_type VARCHAR(64)`, `severity VARCHAR(16)`, `message TEXT`, `metric_name VARCHAR(64)`, `metric_value DOUBLE PRECISION`, `timestamp TIMESTAMP`

---

## 3. High-Performance Analytics Queries

### Recovery Success Rate
```sql
SELECT 
    COUNT(*) AS total_attempts,
    COUNT(*) FILTER (WHERE success = TRUE) AS successful_recoveries,
    COUNT(*) FILTER (WHERE success = FALSE) AS failed_recoveries,
    ROUND((COUNT(*) FILTER (WHERE success = TRUE)::NUMERIC / NULLIF(COUNT(*), 0) * 100), 2) AS success_rate_pct
FROM RECOVERY;
```

### Mean Time to Detect (MTTD) & Recover (MTTR)
```sql
SELECT 
    i.incident_type,
    COUNT(i.id) AS total_incidents,
    ROUND(AVG(EXTRACT(EPOCH FROM (r.completed_at - i.detected_at)))::NUMERIC, 2) AS mean_mttr_seconds
FROM INCIDENT i
JOIN RECOVERY r ON i.id = r.incident_id
WHERE r.success = TRUE
GROUP BY i.incident_type
ORDER BY total_incidents DESC;
```

### Process Resource Drift Over Time
```sql
SELECT 
    pid,
    time_bucket('10 seconds', timestamp) AS bucket,
    AVG(cpu_percent) AS avg_cpu,
    MAX(cpu_percent) AS peak_cpu,
    AVG(memory_rss_mb) AS avg_rss
FROM PROCESS_SNAPSHOT
WHERE pid = :target_pid AND timestamp >= NOW() - INTERVAL '15 minutes'
GROUP BY pid, bucket
ORDER BY bucket ASC;
```
