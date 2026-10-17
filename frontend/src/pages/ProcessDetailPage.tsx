import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { api } from '../services/api';
import { ProcessDetail, ProcessHistoryItem } from '../types';
import { StatusBadge } from '../components/StatusBadge';
import { MetricCard } from '../components/MetricCard';

export const ProcessDetailPage: React.FC = () => {
  const { pid } = useParams<{ pid: string }>();
  const [detail, setDetail] = useState<ProcessDetail | null>(null);
  const [history, setHistory] = useState<ProcessHistoryItem[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [actionMsg, setActionMsg] = useState<string>('');

  const loadData = async () => {
    if (!pid) return;
    try {
      setLoading(true);
      const [d, h] = await Promise.all([
        api.getProcessDetail(Number(pid)),
        api.getProcessHistory(Number(pid))
      ]);
      setDetail(d);
      setHistory(h);
    } catch (err) {
      console.error('Failed to load process details:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 3000);
    return () => clearInterval(interval);
  }, [pid]);

  const handleControl = async (action: string) => {
    if (!pid) return;
    try {
      setActionMsg(`Dispatching ${action}...`);
      const res = await api.controlProcess(Number(pid), action);
      setActionMsg(res.message);
      loadData();
    } catch (err: any) {
      setActionMsg(`Failed: ${err.message}`);
    }
  };

  if (!detail && !loading) {
    return (
      <div className="text-center py-12">
        <h2 className="text-lg font-bold text-white">Process Not Found</h2>
        <p className="text-xs text-slate-400 mt-1">PID {pid} is no longer running or invalid.</p>
        <Link to="/processes" className="mt-4 inline-block px-4 py-2 bg-blue-600 rounded-lg text-xs text-white">
          Back to Processes
        </Link>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header bar */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 bg-slate-900 p-6 rounded-xl border border-slate-800">
        <div>
          <div className="flex items-center space-x-3">
            <h1 className="text-2xl font-bold text-white">{detail?.process_name}</h1>
            <span className="font-mono text-sm px-2 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700">
              PID {detail?.pid}
            </span>
            {detail && <StatusBadge status={detail.status} />}
          </div>
          <p className="text-xs text-slate-400 mt-1 font-mono">
            {detail?.command_line || 'Command line unavailable'}
          </p>
        </div>

        {/* Process Control Actions */}
        <div className="flex flex-wrap items-center gap-2">
          <button
            onClick={() => handleControl('terminate_process')}
            className="px-3 py-1.5 rounded-lg bg-rose-950/80 hover:bg-rose-900 text-rose-300 border border-rose-800 text-xs font-medium transition"
          >
            SIGTERM
          </button>
          <button
            onClick={() => handleControl('restart_process')}
            className="px-3 py-1.5 rounded-lg bg-blue-950/80 hover:bg-blue-900 text-blue-300 border border-blue-800 text-xs font-medium transition"
          >
            Restart
          </button>
          <button
            onClick={() => handleControl('suspend_process')}
            className="px-3 py-1.5 rounded-lg bg-amber-950/80 hover:bg-amber-900 text-amber-300 border border-amber-800 text-xs font-medium transition"
          >
            Pause (SIGSTOP)
          </button>
          <button
            onClick={() => handleControl('resume_process')}
            className="px-3 py-1.5 rounded-lg bg-emerald-950/80 hover:bg-emerald-900 text-emerald-300 border border-emerald-800 text-xs font-medium transition"
          >
            Resume (SIGCONT)
          </button>
          <button
            onClick={() => handleControl('change_priority')}
            className="px-3 py-1.5 rounded-lg bg-purple-950/80 hover:bg-purple-900 text-purple-300 border border-purple-800 text-xs font-medium transition"
          >
            Lower Priority (+10)
          </button>
        </div>
      </div>

      {actionMsg && (
        <div className="p-3 rounded-lg bg-slate-900 border border-blue-800 text-xs text-blue-300">
          {actionMsg}
        </div>
      )}

      {/* Snapshot Cards */}
      <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-4">
        <MetricCard title="CPU Usage" value={`${detail?.cpu_usage || 0}%`} />
        <MetricCard title="RSS Memory" value={`${detail?.memory_usage || 0} MB`} />
        <MetricCard title="Threads" value={detail?.thread_count || 1} />
        <MetricCard title="Priority (Nice)" value={detail?.priority || 0} />
        <MetricCard title="Open FDs" value={detail?.open_file_descriptors || 0} />
        <MetricCard title="Context Switches" value={detail?.context_switches || 0} />
      </div>

      {/* Historical Telemetry Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* CPU History */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <h3 className="text-xs font-bold text-white uppercase tracking-wider mb-2">CPU Utilization History</h3>
          <div className="h-40 w-full flex items-end space-x-1 pt-4">
            {history.length > 0 ? (
              history.map((pt, idx) => (
                <div key={idx} className="flex-1 bg-slate-800/40 rounded-t flex flex-col justify-end h-full group relative">
                  <div
                    className="w-full bg-blue-500 rounded-t transition-all group-hover:bg-blue-400"
                    style={{ height: `${Math.min(100, Math.max(5, pt.cpu_usage))}%` }}
                  />
                  <div className="absolute bottom-full mb-1 hidden group-hover:block bg-slate-950 text-white text-[10px] p-1 rounded border border-slate-800 z-10 whitespace-nowrap">
                    {pt.cpu_usage}%
                  </div>
                </div>
              ))
            ) : (
              <div className="w-full h-full flex items-center justify-center text-slate-500 text-xs">
                No historical samples available
              </div>
            )}
          </div>
        </div>

        {/* Memory History */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <h3 className="text-xs font-bold text-white uppercase tracking-wider mb-2">Memory (RSS) History</h3>
          <div className="h-40 w-full flex items-end space-x-1 pt-4">
            {history.length > 0 ? (
              history.map((pt, idx) => (
                <div key={idx} className="flex-1 bg-slate-800/40 rounded-t flex flex-col justify-end h-full group relative">
                  <div
                    className="w-full bg-emerald-500 rounded-t transition-all group-hover:bg-emerald-400"
                    style={{ height: `${Math.min(100, Math.max(5, (pt.memory_usage / 500) * 100))}%` }}
                  />
                  <div className="absolute bottom-full mb-1 hidden group-hover:block bg-slate-950 text-white text-[10px] p-1 rounded border border-slate-800 z-10 whitespace-nowrap">
                    {pt.memory_usage} MB
                  </div>
                </div>
              ))
            ) : (
              <div className="w-full h-full flex items-center justify-center text-slate-500 text-xs">
                No historical samples available
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
