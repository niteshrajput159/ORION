-- ============================================================================
-- OScope: Core Analytics Queries
-- ============================================================================

-- 1. System Health & Recovery Success Rate
SELECT
    COUNT(*) AS total_incidents,
    COUNT(CASE WHEN status = 'RESOLVED' THEN 1 END) AS resolved_incidents,
    COUNT(CASE WHEN status IN ('OPEN', 'INVESTIGATING', 'RECOVERING') THEN 1 END) AS active_incidents,
    COUNT(CASE WHEN status = 'ESCALATED' THEN 1 END) AS escalated_incidents
FROM INCIDENT;

SELECT
    COUNT(*) AS total_recoveries,
    COUNT(CASE WHEN success = true THEN 1 END) AS successful_recoveries,
    COUNT(CASE WHEN success = false THEN 1 END) AS failed_recoveries,
    ROUND(COALESCE(COUNT(CASE WHEN success = true THEN 1 END)::NUMERIC / NULLIF(COUNT(*), 0) * 100.0, 0.0), 2) AS success_rate_percent,
    ROUND(COALESCE(AVG(duration_ms), 0.0)::NUMERIC, 2) AS avg_duration_ms
FROM RECOVERY;

-- 2. Incidents by Type and Severity
SELECT
    incident_type,
    severity,
    COUNT(*) AS count
FROM INCIDENT
GROUP BY incident_type, severity
ORDER BY count DESC;

-- 3. Top Resource Consuming Processes (Last 5 minutes)
SELECT
    p.process_id,
    p.pid,
    p.process_name,
    AVG(s.cpu_usage) AS avg_cpu,
    MAX(s.cpu_usage) AS max_cpu,
    AVG(s.memory_usage) AS avg_memory_mb,
    MAX(s.memory_usage) AS max_memory_mb,
    MAX(s.open_file_descriptors) AS max_fds
FROM PROCESS p
JOIN PROCESS_SNAPSHOT s ON p.process_id = s.process_id
WHERE s.timestamp >= NOW() - INTERVAL '5 minutes'
GROUP BY p.process_id, p.pid, p.process_name
ORDER BY avg_cpu DESC
LIMIT 10;

-- 4. Historical Process Metrics for Detail Chart
-- Usage: Replace $1 with process_id
SELECT
    timestamp,
    cpu_usage,
    memory_usage,
    thread_count,
    priority,
    context_switches,
    page_faults,
    disk_read,
    disk_write,
    open_file_descriptors
FROM PROCESS_SNAPSHOT
WHERE process_id = $1
ORDER BY timestamp ASC
LIMIT 300;

-- 5. Incident Timeline and Associated Recovery Action
-- Usage: Replace $1 with incident_id
SELECT
    i.incident_id,
    i.incident_type,
    i.severity,
    i.detected_at,
    i.status,
    i.description,
    p.action AS policy_action,
    r.recovery_id,
    r.action AS recovery_action,
    r.attempt_number,
    r.started_at,
    r.completed_at,
    r.result,
    r.success,
    r.verification_details
FROM INCIDENT i
LEFT JOIN ACTION_LOG al ON i.incident_id = al.incident_id
LEFT JOIN POLICY p ON al.policy_id = p.policy_id
LEFT JOIN RECOVERY r ON i.incident_id = r.incident_id
WHERE i.incident_id = $1
ORDER BY r.attempt_number ASC;
