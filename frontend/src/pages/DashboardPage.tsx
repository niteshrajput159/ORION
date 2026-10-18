import React, { useEffect, useState } from 'react';
import { useOutletContext, Link } from 'react-router-dom';
import { MetricCard } from '../components/MetricCard';
import { StatusBadge } from '../components/StatusBadge';
import { api } from '../services/api';
import { SystemHealth, SystemOverviewChart, IncidentItem, RecoveryItem } from '../types';

export const DashboardPage: React.FC = () => {
  const { health } = useOutletContext<{ health: SystemHealth | null }>();
  const [timeline, setTimeline] = useState<SystemOverviewChart[]>([]);
  const [recentIncidents, setRecentIncidents] = useState<IncidentItem[]>([]);
  const [recentRecoveries, setRecentRecoveries] = useState<RecoveryItem[]>([]);
  const [loading, setLoading] = useState<boolean>(true);

  useEffect(() => {
    const loadDashboardData = async () => {
      try {
        const [tl, incs, recs] = await Promise.all([
          api.getSystemOverview(),
          api.getIncidents(undefined, undefined, undefined),
          api.getRecoveries()
        ]);
        setTimeline(tl);
        setRecentIncidents(incs.slice(0, 5));
        setRecentRecoveries(recs.slice(0, 5));
      } catch (err) {
        console.error('Failed to load dashboard data:', err);
      } finally {
        setLoading(false);
      }
    };

    loadDashboardData();
    const interval = setInterval(loadDashboardData, 4000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="space-y-6">
      {/* Top Banner */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 bg-gradient-to-r from-slate-900 via-slate-900 to-blue-950/40 p-6 rounded-2xl border border-slate-800 shadow-xl">
        <div>
          <h1 className="text-2xl font-black tracking-tight text-white flex items-center space-x-3">
            <span>Autonomous Behavior & Recovery</span>
            {health && <StatusBadge status={health.health} />}
          </h1>
          <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
            Real-time closed-loop operating system behavioral surveillance, automated fault-detection, and policy-driven remediation.
          </p>
        </div>
        <div className="flex items-center space-x-3 shrink-0">
          <Link
            to="/workloads"
            className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white font-medium text-xs transition shadow-lg shadow-blue-600/30 flex items-center space-x-1.5"
          >
            <span>Launch Test Workload</span>
            <svg className="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14 5l7 7m0 0l-7 7m7-7H3" />
            </svg>
          </Link>
        </div>
      </div>

      {/* 8 Essential Key Metrics Grid */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <MetricCard
          title="CPU Utilization"
          value={health ? `${health.cpu_usage_percent}%` : '--'}
          subtitle="Real-time multi-core load"
          variant={health && health.cpu_usage_percent > 85 ? 'danger' : 'default'}
          icon={
            <svg className="w-5 h-5 text-blue-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
            </svg>
          }
        />
        <MetricCard
          title="Memory (RSS)"
          value={health ? `${health.memory_usage_percent}%` : '--'}
          subtitle={health ? `${Math.round(health.memory_used_mb)} / ${Math.round(health.memory_total_mb)} MB` : '--'}
          variant={health && health.memory_usage_percent > 85 ? 'warning' : 'default'}
          icon={
            <svg className="w-5 h-5 text-emerald-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" />
            </svg>
          }
        />
        <MetricCard
          title="Active Processes"
          value={health ? health.active_processes : '--'}
          subtitle="Observed via /proc"
          icon={
            <svg className="w-5 h-5 text-cyan-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6h16M4 10h16M4 14h16M4 18h16" />
            </svg>
          }
        />
        <MetricCard
          title="Active Incidents"
          value={health ? health.active_incidents : '--'}
          subtitle="Open / Investigating"
          variant={health && health.active_incidents > 0 ? 'danger' : 'success'}
          icon={
            <svg className="w-5 h-5 text-rose-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
            </svg>
          }
        />
        <MetricCard
          title="Recovery Success"
          value={health ? `${health.recovery_success_rate}%` : '--'}
          subtitle="Verified OS recoveries"
          variant="success"
          icon={
            <svg className="w-5 h-5 text-emerald-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
          }
        />
        <MetricCard
          title="Failed Recoveries"
          value={health ? health.failed_recoveries : '--'}
          subtitle="Escalations / timeouts"
          variant={health && health.failed_recoveries > 0 ? 'warning' : 'default'}
          icon={
            <svg className="w-5 h-5 text-amber-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
          }
        />
        <MetricCard
          title="Swap Memory Used"
          value={health ? `${Math.round(health.swap_used_mb)} MB` : '--'}
          subtitle="Paging activity"
          icon={
            <svg className="w-5 h-5 text-purple-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 7h12m0 0l-4-4m4 4l-4 4m0 6H4m0 0l4 4m-4-4l4-4" />
            </svg>
          }
        />
        <MetricCard
          title="Free Memory"
          value={health ? `${Math.round(health.memory_free_mb)} MB` : '--'}
          subtitle="Available for allocation"
          icon={
            <svg className="w-5 h-5 text-teal-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 15a4 4 0 004 4h9a5 5 0 10-.1-9.999 5.002 5.002 0 00-9.78 2.096A4.001 4.001 0 003 15z" />
            </svg>
          }
        />
      </div>

      {/* Visual Chart: CPU & Memory Timeline */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
        <div className="flex items-center justify-between mb-4">
          <div>
            <h2 className="text-sm font-bold text-white uppercase tracking-wider">System Load Over Time</h2>
            <p className="text-xs text-slate-400">High-resolution multi-sampling trajectory</p>
          </div>
          <div className="flex items-center space-x-4 text-xs">
            <span className="flex items-center space-x-1.5">
              <span className="w-3 h-3 rounded-full bg-blue-500 inline-block" />
              <span className="text-slate-300">CPU %</span>
            </span>
            <span className="flex items-center space-x-1.5">
              <span className="w-3 h-3 rounded-full bg-emerald-500 inline-block" />
              <span className="text-slate-300">Memory %</span>
            </span>
          </div>
        </div>

        {/* Responsive SVG Chart */}
        <div className="h-48 w-full">
          {timeline.length > 0 ? (
            <svg className="w-full h-full overflow-visible" viewBox="0 0 600 160" preserveAspectRatio="none">
              {/* Grid lines */}
              <line x1="0" y1="40" x2="600" y2="40" stroke="#1e293b" strokeDasharray="3,3" />
              <line x1="0" y1="80" x2="600" y2="80" stroke="#1e293b" strokeDasharray="3,3" />
              <line x1="0" y1="120" x2="600" y2="120" stroke="#1e293b" strokeDasharray="3,3" />

              {/* CPU Line */}
              <polyline
                fill="none"
                stroke="#3b82f6"
                strokeWidth="2.5"
                points={timeline
                  .map((pt, i) => `${(i / (timeline.length - 1)) * 600},${160 - (pt.cpu_percent / 100) * 150}`)
                  .join(' ')}
              />

              {/* Memory Line */}
              <polyline
                fill="none"
                stroke="#10b981"
                strokeWidth="2.5"
                points={timeline
                  .map((pt, i) => `${(i / (timeline.length - 1)) * 600},${160 - (pt.memory_percent / 100) * 150}`)
                  .join(' ')}
              />
            </svg>
          ) : (
            <div className="h-full flex items-center justify-center text-slate-500 text-xs">
              Collecting periodic telemetry...
            </div>
          )}
        </div>
      </div>

      {/* Two Column Layout: Recent Incidents vs Recent Recoveries */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Recent Incidents Card */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <div className="flex items-center justify-between pb-3 border-b border-slate-800 mb-4">
            <h2 className="text-sm font-bold text-white uppercase tracking-wider flex items-center space-x-2">
              <span className="w-2 h-2 rounded-full bg-rose-500" />
              <span>Active & Recent Incidents</span>
            </h2>
            <Link to="/incidents" className="text-xs text-blue-400 hover:text-blue-300 font-medium">
              View All ({health?.active_incidents || 0}) →
            </Link>
          </div>

          {recentIncidents.length === 0 ? (
            <div className="py-8 text-center text-slate-500 text-xs">
              No incidents detected. System behavior normal.
            </div>
          ) : (
            <div className="space-y-3">
              {recentIncidents.map((inc) => (
                <Link
                  key={inc.incident_id}
                  to={`/incidents/${inc.incident_id}`}
                  className="block p-3 rounded-lg bg-slate-950/60 border border-slate-800/80 hover:border-slate-700 transition"
                >
                  <div className="flex items-center justify-between">
                    <span className="text-xs font-bold text-white">{inc.incident_type}</span>
                    <StatusBadge status={inc.severity} />
                  </div>
                  <p className="text-xs text-slate-400 mt-1 line-clamp-1">{inc.description}</p>
                  <div className="mt-2 flex items-center justify-between text-[11px] text-slate-500">
                    <span>Process: {inc.process_name} (PID {inc.pid || '?'})</span>
                    <span>Status: {inc.status}</span>
                  </div>
                </Link>
              ))}
            </div>
          )}
        </div>

        {/* Recent Recoveries Card */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <div className="flex items-center justify-between pb-3 border-b border-slate-800 mb-4">
            <h2 className="text-sm font-bold text-white uppercase tracking-wider flex items-center space-x-2">
              <span className="w-2 h-2 rounded-full bg-emerald-500" />
              <span>Recovery Operations Log</span>
            </h2>
            <Link to="/recovery" className="text-xs text-blue-400 hover:text-blue-300 font-medium">
              View All History →
            </Link>
          </div>

          {recentRecoveries.length === 0 ? (
            <div className="py-8 text-center text-slate-500 text-xs">
              No recovery actions executed yet.
            </div>
          ) : (
            <div className="space-y-3">
              {recentRecoveries.map((rec) => (
                <div
                  key={rec.recovery_id}
                  className="p-3 rounded-lg bg-slate-950/60 border border-slate-800/80 flex items-center justify-between"
                >
                  <div>
                    <div className="flex items-center space-x-2">
                      <span className="text-xs font-bold text-white">{rec.action}</span>
                      <span className="text-[10px] text-slate-500 font-mono">#{rec.recovery_id}</span>
                    </div>
                    <p className="text-xs text-slate-400 mt-0.5 line-clamp-1">
                      {rec.verification_details || rec.result}
                    </p>
                  </div>
                  <div className="text-right shrink-0 ml-4">
                    <StatusBadge status={rec.success ? 'SUCCESS' : 'FAILED'} />
                    <span className="block text-[10px] text-slate-500 mt-1">{rec.duration_ms}ms</span>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
