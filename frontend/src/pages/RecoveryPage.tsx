import React, { useEffect, useState } from 'react';
import { api } from '../services/api';
import { RecoveryItem, RecoveryChartMetrics } from '../types';
import { StatusBadge } from '../components/StatusBadge';
import { MetricCard } from '../components/MetricCard';

export const RecoveryPage: React.FC = () => {
  const [recoveries, setRecoveries] = useState<RecoveryItem[]>([]);
  const [metrics, setMetrics] = useState<RecoveryChartMetrics | null>(null);
  const [loading, setLoading] = useState<boolean>(true);

  const fetchRecoveryData = async () => {
    try {
      setLoading(true);
      const [recs, mets] = await Promise.all([
        api.getRecoveries(),
        api.getRecoveryMetrics()
      ]);
      setRecoveries(recs);
      setMetrics(mets);
    } catch (err) {
      console.error('Failed to load recovery data:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchRecoveryData();
    const interval = setInterval(fetchRecoveryData, 4000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-xl font-bold text-white tracking-tight">Recovery Operations Center</h1>
        <p className="text-xs text-slate-400 mt-0.5">
          Audited log of controlled POSIX actions, verification polling cycles, and escalation traces
        </p>
      </div>

      {/* Analytics Summary Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-4">
        <MetricCard
          title="Successful Recoveries"
          value={metrics ? metrics.successful_count : '--'}
          subtitle="Verified OS restorations"
          variant="success"
        />
        <MetricCard
          title="Failed Recoveries"
          value={metrics ? metrics.failed_count : '--'}
          subtitle="Exceeded bounds / timeouts"
          variant={metrics && metrics.failed_count > 0 ? 'danger' : 'default'}
        />
        <MetricCard
          title="Avg Recovery Duration"
          value={metrics ? `${metrics.avg_duration_ms} ms` : '--'}
          subtitle="Action execution + verification latency"
        />
        <MetricCard
          title="Attempts / Incident"
          value={metrics ? metrics.attempts_per_incident : '--'}
          subtitle="Mean stabilization cycles"
        />
      </div>

      {/* Recovery Table */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl">
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-xs">
            <thead>
              <tr className="bg-slate-950/80 border-b border-slate-800 text-slate-400 font-semibold uppercase tracking-wider text-[11px]">
                <th className="py-3 px-4">Recovery ID</th>
                <th className="py-3 px-4">Incident ID</th>
                <th className="py-3 px-4">Action</th>
                <th className="py-3 px-4">Attempt</th>
                <th className="py-3 px-4">Result</th>
                <th className="py-3 px-4">Success</th>
                <th className="py-3 px-4">Duration</th>
                <th className="py-3 px-4">Started At</th>
                <th className="py-3 px-4">Verification Details</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-mono">
              {recoveries.length === 0 ? (
                <tr>
                  <td colSpan={9} className="py-8 text-center text-slate-500 font-sans">
                    {loading ? 'Loading recovery audit log...' : 'No recovery operations recorded yet.'}
                  </td>
                </tr>
              ) : (
                recoveries.map((rec) => (
                  <tr key={rec.recovery_id} className="hover:bg-slate-800/40 transition">
                    <td className="py-3 px-4 font-bold text-white">#{rec.recovery_id}</td>
                    <td className="py-3 px-4 text-blue-400">#{rec.incident_id}</td>
                    <td className="py-3 px-4 font-semibold text-white">{rec.action}</td>
                    <td className="py-3 px-4 text-slate-300">#{rec.attempt_number}</td>
                    <td className="py-3 px-4 text-slate-300">{rec.result}</td>
                    <td className="py-3 px-4 font-sans">
                      <StatusBadge status={rec.success ? 'SUCCESS' : 'FAILED'} />
                    </td>
                    <td className="py-3 px-4 text-slate-300">{rec.duration_ms}ms</td>
                    <td className="py-3 px-4 text-slate-400 text-[11px]">
                      {new Date(rec.started_at).toLocaleTimeString()}
                    </td>
                    <td className="py-3 px-4 font-sans text-slate-400 text-[11px] max-w-xs truncate">
                      {rec.verification_details || '--'}
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
