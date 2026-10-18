import React, { useEffect, useState } from 'react';
import { api } from '../services/api';
import { ResourceHistoricalPoint } from '../types';
import { MetricCard } from '../components/MetricCard';

export const ResourcesPage: React.FC = () => {
  const [current, setCurrent] = useState<any>(null);
  const [history, setHistory] = useState<ResourceHistoricalPoint[]>([]);
  const [loading, setLoading] = useState<boolean>(true);

  const fetchResources = async () => {
    try {
      setLoading(true);
      const [curr, hist] = await Promise.all([
        api.getCurrentResources(),
        api.getResourceHistory()
      ]);
      setCurrent(curr);
      setHistory(hist);
    } catch (err) {
      console.error('Failed to load resources:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchResources();
    const interval = setInterval(fetchResources, 4000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-xl font-bold text-white tracking-tight">System Resource Analytics</h1>
        <p className="text-xs text-slate-400 mt-0.5">
          Real-time kernel resource metrics: CPU, memory, threads, file descriptors, and disk I/O
        </p>
      </div>

      {/* Resource Metrics Cards */}
      <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-4">
        <MetricCard title="System CPU" value={current ? `${current.cpu_percent}%` : '--'} />
        <MetricCard title="System Memory" value={current ? `${current.memory_percent}%` : '--'} />
        <MetricCard title="Total Threads" value={current ? current.total_threads : '--'} />
        <MetricCard title="Open File Descriptors" value={current ? current.total_open_fds : '--'} />
        <MetricCard title="Process Count" value={current ? current.total_processes : '--'} />
        <MetricCard title="Swap Used" value={current ? `${Math.round(current.swap_used_mb)} MB` : '--'} />
      </div>

      {/* Historical Resource Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Threads vs FDs Chart */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h3 className="text-sm font-bold text-white uppercase tracking-wider">Active Threads & File Descriptors</h3>
              <p className="text-xs text-slate-400">Concurrency footprint over time</p>
            </div>
            <div className="flex items-center space-x-3 text-xs">
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-cyan-500 inline-block" />
                <span className="text-slate-300">Threads</span>
              </span>
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-amber-500 inline-block" />
                <span className="text-slate-300">Open FDs</span>
              </span>
            </div>
          </div>

          <div className="h-44 w-full pt-4">
            {history.length > 0 ? (
              <svg className="w-full h-full overflow-visible" viewBox="0 0 600 140" preserveAspectRatio="none">
                <line x1="0" y1="35" x2="600" y2="35" stroke="#1e293b" strokeDasharray="3,3" />
                <line x1="0" y1="70" x2="600" y2="70" stroke="#1e293b" strokeDasharray="3,3" />
                <line x1="0" y1="105" x2="600" y2="105" stroke="#1e293b" strokeDasharray="3,3" />

                {/* Threads Line */}
                <polyline
                  fill="none"
                  stroke="#06b6d4"
                  strokeWidth="2.5"
                  points={history
                    .map((pt, i) => `${(i / (history.length - 1)) * 600},${140 - Math.min(130, pt.threads * 1.5)}`)
                    .join(' ')}
                />

                {/* FDs Line */}
                <polyline
                  fill="none"
                  stroke="#f59e0b"
                  strokeWidth="2"
                  points={history
                    .map((pt, i) => `${(i / (history.length - 1)) * 600},${140 - Math.min(130, pt.open_fds * 0.8)}`)
                    .join(' ')}
                />
              </svg>
            ) : (
              <div className="h-full flex items-center justify-center text-slate-500 text-xs">
                No historical resource samples
              </div>
            )}
          </div>
        </div>

        {/* Process Count Trajectory */}
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-lg">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h3 className="text-sm font-bold text-white uppercase tracking-wider">Process Lifecycle Count</h3>
              <p className="text-xs text-slate-400">Total registered and active tasks</p>
            </div>
          </div>

          <div className="h-44 w-full pt-4">
            {history.length > 0 ? (
              <svg className="w-full h-full overflow-visible" viewBox="0 0 600 140" preserveAspectRatio="none">
                <line x1="0" y1="35" x2="600" y2="35" stroke="#1e293b" strokeDasharray="3,3" />
                <line x1="0" y1="70" x2="600" y2="70" stroke="#1e293b" strokeDasharray="3,3" />
                <line x1="0" y1="105" x2="600" y2="105" stroke="#1e293b" strokeDasharray="3,3" />

                <polyline
                  fill="none"
                  stroke="#a855f7"
                  strokeWidth="2.5"
                  points={history
                    .map((pt, i) => `${(i / (history.length - 1)) * 600},${140 - Math.min(130, pt.process_count * 2.5)}`)
                    .join(' ')}
                />
              </svg>
            ) : (
              <div className="h-full flex items-center justify-center text-slate-500 text-xs">
                No historical resource samples
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
