import {
  SystemHealth, SystemOverviewChart,
  ProcessItem, ProcessDetail, ProcessHistoryItem,
  IncidentItem, IncidentDetail,
  PolicyItem,
  RecoveryItem, RecoveryChartMetrics,
  ResourceEventItem, ResourceHistoricalPoint,
  DeadlockReport,
  AnalyticsSummary
} from '../types';

const API_BASE = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8000';

async function fetchJson<T>(endpoint: string, options?: RequestInit): Promise<T> {
  const url = `${API_BASE}${endpoint}`;
  try {
    const res = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...(options?.headers || {})
      }
    });
    if (!res.ok) {
      const errText = await res.text();
      throw new Error(`API Error [${res.status}]: ${errText || res.statusText}`);
    }
    return (await res.json()) as T;
  } catch (err: any) {
    console.error(`Fetch failed for ${url}:`, err);
    throw err;
  }
}

export const api = {
  // System
  getSystemHealth: () => fetchJson<SystemHealth>('/api/system/health'),
  getSystemOverview: () => fetchJson<SystemOverviewChart[]>('/api/system/overview'),

  // Processes
  getProcesses: (search?: string, status?: string) => {
    const params = new URLSearchParams();
    if (search) params.append('search', search);
    if (status) params.append('status', status);
    const qs = params.toString() ? `?${params.toString()}` : '';
    return fetchJson<ProcessItem[]>(`/api/processes${qs}`);
  },
  getProcessDetail: (pid: number) => fetchJson<ProcessDetail>(`/api/processes/${pid}`),
  getProcessHistory: (pid: number) => fetchJson<ProcessHistoryItem[]>(`/api/processes/${pid}/history`),
  controlProcess: (pid: number, action: string, niceValue?: number, mode?: string) =>
    fetchJson<{ success: boolean; message: string; action: string }>(`/api/processes/${pid}/control`, {
      method: 'POST',
      body: JSON.stringify({ action, nice_value: niceValue, mode })
    }),

  // Incidents
  getIncidents: (type?: string, severity?: string, status?: string) => {
    const params = new URLSearchParams();
    if (type) params.append('incident_type', type);
    if (severity) params.append('severity', severity);
    if (status) params.append('status', status);
    const qs = params.toString() ? `?${params.toString()}` : '';
    return fetchJson<IncidentItem[]>(`/api/incidents${qs}`);
  },
  getIncidentDetail: (id: number) => fetchJson<IncidentDetail>(`/api/incidents/${id}`),
  getAiExplanation: (id: number) => fetchJson<any>(`/api/incidents/${id}/explain`),
  resolveIncident: (id: number) => fetchJson<any>(`/api/incidents/${id}/resolve`, { method: 'POST' }),
  recoverIncident: (id: number) => fetchJson<any>(`/api/incidents/${id}/recover`, { method: 'POST' }),
  resolveAllIncidents: () => fetchJson<any>('/api/incidents/resolve-all', { method: 'POST' }),

  // Recovery
  getRecoveries: () => fetchJson<RecoveryItem[]>('/api/recovery'),
  getRecoveryMetrics: () => fetchJson<RecoveryChartMetrics>('/api/recovery/metrics'),

  // Resources
  getCurrentResources: () => fetchJson<any>('/api/resources/current'),
  getResourceHistory: () => fetchJson<ResourceHistoricalPoint[]>('/api/resources/history'),
  getEvents: (resourceType?: string, severity?: string) => {
    const params = new URLSearchParams();
    if (resourceType) params.append('resource_type', resourceType);
    if (severity) params.append('severity', severity);
    const qs = params.toString() ? `?${params.toString()}` : '';
    return fetchJson<ResourceEventItem[]>(`/api/events${qs}`);
  },

  // Deadlocks
  getDeadlockReport: (stateFile?: string) => {
    const qs = stateFile ? `?state_file=${encodeURIComponent(stateFile)}` : '';
    return fetchJson<DeadlockReport>(`/api/deadlocks${qs}`);
  },
  resolveDeadlock: (strategy?: string) =>
    fetchJson<{ success: boolean; message: string; incident_id?: number; recovery_id?: number }>('/api/deadlocks/resolve', {
      method: 'POST',
      body: JSON.stringify({ strategy: strategy || 'LEAST_COST' })
    }),

  // Policies
  getPolicies: () => fetchJson<PolicyItem[]>('/api/policies'),
  updatePolicy: (id: number, patch: Partial<PolicyItem>) =>
    fetchJson<PolicyItem>(`/api/policies/${id}`, {
      method: 'PATCH',
      body: JSON.stringify(patch)
    }),

  // Analytics
  getAnalyticsSummary: () => fetchJson<AnalyticsSummary>('/api/analytics/summary'),

  // Workloads
  getWorkloads: () => fetchJson<Record<string, { running: boolean; pid: number | null }>>('/api/workloads'),
  startWorkload: (name: string, args?: string[]) => {
    const qs = args && args.length ? `?${args.map(a => `args=${encodeURIComponent(a)}`).join('&')}` : '';
    return fetchJson<any>(`/api/workloads/${name}/start${qs}`, { method: 'POST' });
  },
  stopWorkload: (name: string) => fetchJson<any>(`/api/workloads/${name}/stop`, { method: 'POST' }),
  remediateWorkload: (name: string) => fetchJson<any>(`/api/workloads/${name}/recover`, { method: 'POST' })
};
