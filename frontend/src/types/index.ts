export interface SystemHealth {
  health: 'HEALTHY' | 'WARNING' | 'CRITICAL';
  cpu_usage_percent: number;
  memory_usage_percent: number;
  memory_total_mb: number;
  memory_used_mb: number;
  memory_free_mb: number;
  swap_used_mb: number;
  active_processes: number;
  active_incidents: number;
  recovery_success_rate: number;
  failed_recoveries: number;
}

export interface SystemOverviewChart {
  timestamp: string;
  cpu_percent: number;
  memory_percent: number;
}

export interface ProcessItem {
  process_id: number;
  pid: number;
  process_name: string;
  ppid: number;
  owner: string;
  start_time?: string;
  status: string;
  is_registered: boolean;
  cpu_usage: number;
  memory_usage: number;
  thread_count: number;
  priority: number;
  state: string;
}

export interface ProcessDetail {
  process_id: number;
  pid: number;
  process_name: string;
  ppid: number;
  owner: string;
  command_line?: string;
  start_time?: string;
  status: string;
  is_registered: boolean;
  cpu_usage: number;
  memory_usage: number;
  virtual_memory: number;
  thread_count: number;
  priority: number;
  state: string;
  context_switches: number;
  page_faults: number;
  open_file_descriptors: number;
}

export interface ProcessHistoryItem {
  timestamp: string;
  cpu_usage: number;
  memory_usage: number;
  thread_count: number;
  priority: number;
  context_switches: number;
  page_faults: number;
  disk_read: number;
  disk_write: number;
  open_file_descriptors: number;
}

export interface IncidentItem {
  incident_id: number;
  process_id?: number;
  pid?: number;
  process_name?: string;
  incident_type: string;
  severity: 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';
  detected_at: string;
  status: 'OPEN' | 'INVESTIGATING' | 'RECOVERING' | 'RESOLVED' | 'ESCALATED' | 'FAILED';
  description?: string;
  recovery_status?: string;
}

export interface TimelineStep {
  step_title: string;
  timestamp: string;
  status: 'COMPLETED' | 'IN_PROGRESS' | 'FAILED' | 'PENDING';
  description: string;
}

export interface IncidentDetail {
  incident_id: number;
  process_id?: number;
  pid?: number;
  process_name?: string;
  incident_type: string;
  severity: 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';
  detected_at: string;
  resolved_at?: string;
  status: string;
  description?: string;
  metadata?: Record<string, any>;
  timeline: TimelineStep[];
  policy_name?: string;
  action_taken?: string;
  attempts: number;
  verification_result?: string;
  final_result?: string;
}

export interface PolicyItem {
  policy_id: number;
  incident_type: string;
  severity: string;
  action: string;
  priority: number;
  max_attempts: number;
  timeout_seconds: number;
  cpu_threshold: number;
  memory_threshold: number;
  persistence_count: number;
  enabled: boolean;
}

export interface RecoveryItem {
  recovery_id: number;
  incident_id: number;
  attempt_number: number;
  action: string;
  started_at: string;
  completed_at?: string;
  result: string;
  success: boolean;
  verification_details?: string;
  duration_ms: number;
}

export interface RecoveryChartMetrics {
  successful_count: number;
  failed_count: number;
  avg_duration_ms: number;
  attempts_per_incident: number;
}

export interface ResourceEventItem {
  event_id: number;
  process_id?: number;
  pid?: number;
  process_name?: string;
  event_type: string;
  resource_type: string;
  value: number;
  threshold: number;
  timestamp: string;
  severity: string;
  message?: string;
}

export interface ResourceHistoricalPoint {
  timestamp: string;
  cpu_percent: number;
  memory_percent: number;
  threads: number;
  open_fds: number;
  process_count: number;
}

export interface DeadlockGraphEdge {
  waiting_thread: number;
  holding_thread: number;
  resource: string;
}

export interface DeadlockReport {
  has_deadlock: boolean;
  threads: number[];
  edges: DeadlockGraphEdge[];
  cycle: number[];
  victim_thread: number;
  strategy: string;
  action: string;
  verification: string;
  final_result: string;
}

export interface AnalyticsSummary {
  total_incidents: number;
  active_incidents: number;
  resolved_incidents: number;
  escalated_incidents: number;
  total_recoveries: number;
  recovery_success_rate: number;
  avg_recovery_duration_ms: number;
  incident_types: { incident_type: string; count: number }[];
  severities: { severity: string; count: number }[];
  recovery_outcomes: {
    action: string;
    success_count: number;
    failure_count: number;
    avg_duration_ms: number;
  }[];
}
