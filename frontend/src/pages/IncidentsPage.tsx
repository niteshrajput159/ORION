import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { api } from '../services/api';
import { IncidentItem } from '../types';
import { StatusBadge } from '../components/StatusBadge';

const INCIDENT_TYPES = [
  'HIGH_CPU',
  'HIGH_MEMORY',
  'PROCESS_CRASH',
  'CRASH_LOOP',
  'HUNG_PROCESS',
  'DEADLOCK',
  'HIGH_IO',
  'EXCESSIVE_THREADS',
  'EXCESSIVE_FILE_DESCRIPTORS'
];

export const IncidentsPage: React.FC = () => {
  const [incidents, setIncidents] = useState<IncidentItem[]>([]);
  const [typeFilter, setTypeFilter] = useState<string>('');
  const [severityFilter, setSeverityFilter] = useState<string>('');
  const [statusFilter, setStatusFilter] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(true);

  const fetchIncidents = async () => {
    try {
      setLoading(true);
      const data = await api.getIncidents(typeFilter, severityFilter, statusFilter);
      setIncidents(data);
    } catch (err) {
      console.error('Failed to load incidents:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchIncidents();
    const interval = setInterval(fetchIncidents, 4000);
    return () => clearInterval(interval);
  }, [typeFilter, severityFilter, statusFilter]);

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl font-bold text-white tracking-tight">Incident Surveillance Center</h1>
          <p className="text-xs text-slate-400 mt-0.5">
            Continuous detection records for threshold, crash, deadlock, and resource violations
          </p>
        </div>

        {/* Filter Controls */}
        <div className="flex flex-wrap items-center gap-2">
          <button
            onClick={async () => {
              if (window.confirm('Resolve all open incidents across the operating system?')) {
                await api.resolveAllIncidents();
                fetchIncidents();
              }
            }}
            className="px-3 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white font-medium text-xs transition shadow-md shadow-emerald-600/30 flex items-center gap-1"
          >
            Resolve All
          </button>

          <select
            value={typeFilter}
            onChange={(e) => setTypeFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All Incident Types</option>
            {INCIDENT_TYPES.map((t) => (
              <option key={t} value={t}>{t}</option>
            ))}
          </select>

          <select
            value={severityFilter}
            onChange={(e) => setSeverityFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All Severities</option>
            <option value="CRITICAL">CRITICAL</option>
            <option value="HIGH">HIGH</option>
            <option value="MEDIUM">MEDIUM</option>
            <option value="LOW">LOW</option>
          </select>

          <select
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All Statuses</option>
            <option value="OPEN">OPEN</option>
            <option value="INVESTIGATING">INVESTIGATING</option>
            <option value="RECOVERING">RECOVERING</option>
            <option value="RESOLVED">RESOLVED</option>
            <option value="ESCALATED">ESCALATED</option>
            <option value="FAILED">FAILED</option>
          </select>
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl">
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-xs">
            <thead>
              <tr className="bg-slate-950/80 border-b border-slate-800 text-slate-400 font-semibold uppercase tracking-wider text-[11px]">
                <th className="py-3 px-4">ID</th>
                <th className="py-3 px-4">Process</th>
                <th className="py-3 px-4">Incident Type</th>
                <th className="py-3 px-4">Severity</th>
                <th className="py-3 px-4">Status</th>
                <th className="py-3 px-4">Recovery Status</th>
                <th className="py-3 px-4">Detected At</th>
                <th className="py-3 px-4 text-right">Actions</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-sans">
              {incidents.length === 0 ? (
                <tr>
                  <td colSpan={8} className="py-8 text-center text-slate-500">
                    {loading ? 'Fetching incidents...' : 'No incidents match selected filters.'}
                  </td>
                </tr>
              ) : (
                incidents.map((inc) => (
                  <tr key={inc.incident_id} className="hover:bg-slate-800/40 transition">
                    <td className="py-3 px-4 font-mono font-bold text-white">#{inc.incident_id}</td>
                    <td className="py-3 px-4 font-medium text-slate-200">
                      {inc.process_name} <span className="text-slate-500 font-mono text-[11px]">(PID {inc.pid || '?'})</span>
                    </td>
                    <td className="py-3 px-4 font-mono font-semibold text-white">
                      {inc.incident_type}
                    </td>
                    <td className="py-3 px-4">
                      <StatusBadge status={inc.severity} />
                    </td>
                    <td className="py-3 px-4">
                      <StatusBadge status={inc.status} />
                    </td>
                    <td className="py-3 px-4 font-mono text-[11px]">
                      {inc.recovery_status === 'SUCCESS' || inc.status === 'RESOLVED' ? (
                        <span className="text-emerald-400">RESOLVED</span>
                      ) : inc.recovery_status === 'FAILED' ? (
                        <span className="text-rose-400">FAILED</span>
                      ) : (
                        <span className="text-amber-400">IN PROGRESS</span>
                      )}
                    </td>
                    <td className="py-3 px-4 text-slate-400 font-mono text-[11px]">
                      {new Date(inc.detected_at).toLocaleTimeString()}
                    </td>
                    <td className="py-3 px-4 text-right">
                      <div className="flex items-center justify-end gap-1.5">
                        {inc.status !== 'RESOLVED' && (
                          <>
                            <button
                              onClick={async () => {
                                await api.recoverIncident(inc.incident_id);
                                fetchIncidents();
                              }}
                              className="px-2 py-1 rounded bg-indigo-600 hover:bg-indigo-500 text-white text-[11px] font-semibold transition"
                              title="Execute automated remediation policy"
                            >
                              Remediate
                            </button>
                            <button
                              onClick={async () => {
                                await api.resolveIncident(inc.incident_id);
                                fetchIncidents();
                              }}
                              className="px-2 py-1 rounded bg-emerald-600 hover:bg-emerald-500 text-white text-[11px] font-semibold transition"
                              title="Mark resolved"
                            >
                              Resolve
                            </button>
                          </>
                        )}
                        <Link
                          to={`/incidents/${inc.incident_id}`}
                          className="px-2.5 py-1 rounded bg-slate-800 hover:bg-blue-600 hover:text-white text-slate-300 text-xs transition border border-slate-700"
                        >
                          Timeline →
                        </Link>
                      </div>
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
