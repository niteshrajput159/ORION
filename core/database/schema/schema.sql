-- ============================================================================
-- OScope: Intelligent OS Behavior & Recovery
-- PostgreSQL Database Schema
-- ============================================================================

-- Drop existing tables if re-creating
DROP TABLE IF EXISTS RECOVERY CASCADE;
DROP TABLE IF EXISTS ACTION_LOG CASCADE;
DROP TABLE IF EXISTS RESOURCE_EVENT CASCADE;
DROP TABLE IF EXISTS INCIDENT CASCADE;
DROP TABLE IF EXISTS POLICY CASCADE;
DROP TABLE IF EXISTS PROCESS_SNAPSHOT CASCADE;
DROP TABLE IF EXISTS PROCESS CASCADE;

-- ----------------------------------------------------------------------------
-- Table: PROCESS
-- Owned by Member 1 (Process & CPU)
-- Represents tracked OS processes (both registered test workloads & system procs)
-- ----------------------------------------------------------------------------
CREATE TABLE PROCESS (
    process_id SERIAL PRIMARY KEY,
    pid INTEGER NOT NULL,
    process_name VARCHAR(255) NOT NULL,
    ppid INTEGER DEFAULT 0,
    owner VARCHAR(64) DEFAULT 'root',
    start_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    status VARCHAR(32) DEFAULT 'RUNNING', -- RUNNING, SLEEPING, STOPPED, ZOMBIE, TERMINATED, CRASHED
    is_registered BOOLEAN DEFAULT TRUE,   -- Only registered workloads are subject to automatic destructive recovery
    command_line TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_process_pid CHECK (pid >= 0)
);

CREATE INDEX idx_process_pid ON PROCESS(pid);
CREATE INDEX idx_process_status ON PROCESS(status);
CREATE INDEX idx_process_name ON PROCESS(process_name);

-- ----------------------------------------------------------------------------
-- Table: PROCESS_SNAPSHOT
-- Owned by Member 1 (Process & CPU)
-- Periodic metric observations captured directly from Linux /proc
-- ----------------------------------------------------------------------------
CREATE TABLE PROCESS_SNAPSHOT (
    snapshot_id BIGSERIAL PRIMARY KEY,
    process_id INTEGER NOT NULL REFERENCES PROCESS(process_id) ON DELETE CASCADE,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    cpu_usage DOUBLE PRECISION NOT NULL DEFAULT 0.0,      -- percentage (0.0 to 100.0 * num_cpus)
    memory_usage DOUBLE PRECISION NOT NULL DEFAULT 0.0,   -- Resident Set Size (RSS) in MegaBytes
    virtual_memory DOUBLE PRECISION NOT NULL DEFAULT 0.0, -- Virtual memory in MegaBytes
    process_state VARCHAR(16) DEFAULT 'R',               -- R (running), S (sleeping), D (disk sleep), Z (zombie), T (stopped)
    thread_count INTEGER NOT NULL DEFAULT 1,
    priority INTEGER NOT NULL DEFAULT 0,                  -- nice value (-20 to 19) or realtime priority
    context_switches BIGINT NOT NULL DEFAULT 0,          -- voluntary + involuntary context switches
    page_faults BIGINT NOT NULL DEFAULT 0,               -- minor + major page faults
    disk_read BIGINT NOT NULL DEFAULT 0,                 -- bytes read from disk
    disk_write BIGINT NOT NULL DEFAULT 0,                -- bytes written to disk
    open_file_descriptors INTEGER NOT NULL DEFAULT 0,    -- count of open file descriptors
    CONSTRAINT chk_snapshot_cpu CHECK (cpu_usage >= 0.0),
    CONSTRAINT chk_snapshot_memory CHECK (memory_usage >= 0.0)
);

CREATE INDEX idx_snapshot_process_time ON PROCESS_SNAPSHOT(process_id, timestamp DESC);
CREATE INDEX idx_snapshot_timestamp ON PROCESS_SNAPSHOT(timestamp DESC);

-- ----------------------------------------------------------------------------
-- Table: INCIDENT
-- Owned by Member 3 (Fault & Deadlock)
-- Records detected anomalies, resource violations, faults, and deadlocks
-- ----------------------------------------------------------------------------
CREATE TABLE INCIDENT (
    incident_id SERIAL PRIMARY KEY,
    process_id INTEGER REFERENCES PROCESS(process_id) ON DELETE SET NULL,
    incident_type VARCHAR(64) NOT NULL, -- HIGH_CPU, HIGH_MEMORY, PROCESS_CRASH, CRASH_LOOP, HUNG_PROCESS, DEADLOCK, HIGH_IO, EXCESSIVE_THREADS, EXCESSIVE_FILE_DESCRIPTORS
    severity VARCHAR(32) NOT NULL DEFAULT 'MEDIUM', -- LOW, MEDIUM, HIGH, CRITICAL
    detected_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    status VARCHAR(32) NOT NULL DEFAULT 'OPEN', -- OPEN, INVESTIGATING, RECOVERING, RESOLVED, ESCALATED, FAILED
    description TEXT,
    resolved_at TIMESTAMP WITH TIME ZONE NULL,
    metadata JSONB DEFAULT '{}'::jsonb
);

CREATE INDEX idx_incident_status ON INCIDENT(status);
CREATE INDEX idx_incident_type ON INCIDENT(incident_type);
CREATE INDEX idx_incident_severity ON INCIDENT(severity);
CREATE INDEX idx_incident_process ON INCIDENT(process_id);
CREATE INDEX idx_incident_detected_at ON INCIDENT(detected_at DESC);

-- ----------------------------------------------------------------------------
-- Table: POLICY
-- Owned by Member 4 (Decision Engine)
-- Declarative rules mapping incident conditions to automated recovery actions
-- ----------------------------------------------------------------------------
CREATE TABLE POLICY (
    policy_id SERIAL PRIMARY KEY,
    incident_type VARCHAR(64) NOT NULL,
    severity VARCHAR(32) NOT NULL DEFAULT 'MEDIUM',
    action VARCHAR(64) NOT NULL, -- LOWER_PRIORITY, SUSPEND, RESUME, RESTART, TERMINATE_SIGTERM, TERMINATE_SIGKILL, RESOLVE_DEADLOCK, ALERT_ONLY
    priority INTEGER NOT NULL DEFAULT 10,
    max_attempts INTEGER NOT NULL DEFAULT 3,
    timeout_seconds INTEGER NOT NULL DEFAULT 10,
    cpu_threshold DOUBLE PRECISION DEFAULT 90.0,
    memory_threshold DOUBLE PRECISION DEFAULT 85.0,
    persistence_count INTEGER DEFAULT 4,
    enabled BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_policy_type_severity ON POLICY(incident_type, severity);
CREATE INDEX idx_policy_enabled ON POLICY(enabled);

-- ----------------------------------------------------------------------------
-- Table: ACTION_LOG
-- Owned by Member 4 (Decision Engine)
-- Audit log of decisions made and actions requested
-- ----------------------------------------------------------------------------
CREATE TABLE ACTION_LOG (
    action_id SERIAL PRIMARY KEY,
    incident_id INTEGER NOT NULL REFERENCES INCIDENT(incident_id) ON DELETE CASCADE,
    policy_id INTEGER REFERENCES POLICY(policy_id) ON DELETE SET NULL,
    action VARCHAR(64) NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    result VARCHAR(32) NOT NULL, -- SUCCESS, FAILED, TIMED_OUT, ESCALATED
    error_message TEXT NULL,
    details JSONB DEFAULT '{}'::jsonb
);

CREATE INDEX idx_action_incident ON ACTION_LOG(incident_id);
CREATE INDEX idx_action_policy ON ACTION_LOG(policy_id);
CREATE INDEX idx_action_timestamp ON ACTION_LOG(timestamp DESC);

-- ----------------------------------------------------------------------------
-- Table: RECOVERY
-- Owned by Member 3 (Fault & Deadlock)
-- Records actual OS recovery execution attempts, outcomes, and verifications
-- ----------------------------------------------------------------------------
CREATE TABLE RECOVERY (
    recovery_id SERIAL PRIMARY KEY,
    incident_id INTEGER NOT NULL REFERENCES INCIDENT(incident_id) ON DELETE CASCADE,
    attempt_number INTEGER NOT NULL DEFAULT 1,
    action VARCHAR(64) NOT NULL,
    started_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP WITH TIME ZONE NULL,
    result VARCHAR(64) NOT NULL, -- VERIFIED_SUCCESS, VERIFIED_FAILURE, TIMED_OUT, ESCALATED_SIGKILL
    success BOOLEAN NOT NULL DEFAULT FALSE,
    verification_details TEXT NULL,
    duration_ms DOUBLE PRECISION DEFAULT 0.0
);

CREATE INDEX idx_recovery_incident ON RECOVERY(incident_id);
CREATE INDEX idx_recovery_success ON RECOVERY(success);
CREATE INDEX idx_recovery_started_at ON RECOVERY(started_at DESC);

-- ----------------------------------------------------------------------------
-- Table: RESOURCE_EVENT
-- Owned by Member 2 (Memory & Resource)
-- Granular threshold crossing events for memory, swap, disk I/O, threads, and FDs
-- ----------------------------------------------------------------------------
CREATE TABLE RESOURCE_EVENT (
    event_id BIGSERIAL PRIMARY KEY,
    process_id INTEGER REFERENCES PROCESS(process_id) ON DELETE CASCADE,
    event_type VARCHAR(64) NOT NULL,   -- THRESHOLD_EXCEEDED, SPIKE_DETECTED, LEAK_DETECTED
    resource_type VARCHAR(64) NOT NULL, -- CPU, MEMORY, SWAP, DISK_IO, THREADS, FILE_DESCRIPTORS
    value DOUBLE PRECISION NOT NULL,
    threshold DOUBLE PRECISION NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    severity VARCHAR(32) NOT NULL DEFAULT 'WARNING', -- INFO, WARNING, CRITICAL
    message TEXT
);

CREATE INDEX idx_resource_event_process ON RESOURCE_EVENT(process_id);
CREATE INDEX idx_resource_event_type ON RESOURCE_EVENT(resource_type);
CREATE INDEX idx_resource_event_timestamp ON RESOURCE_EVENT(timestamp DESC);
