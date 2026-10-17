import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { api } from '../services/api';
import { ProcessItem } from '../types';
import { StatusBadge } from '../components/StatusBadge';

export const ProcessesPage: React.FC = () => {
  const [processes, setProcesses] = useState<ProcessItem[]>([]);
  const [search, setSearch] = useState<string>('');
  const [statusFilter, setStatusFilter] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(true);
  const [sortField, setSortField] = useState<keyof ProcessItem>('cpu_usage');
  const [sortAsc, setSortAsc] = useState<boolean>(false);

  const fetchProcesses = async () => {
    try {
      setLoading(true);
      const data = await api.getProcesses(search, statusFilter);
      setProcesses(data);
    } catch (err) {
      console.error('Failed to load processes:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchProcesses();
    const interval = setInterval(fetchProcesses, 4000);
    return () => clearInterval(interval);
  }, [search, statusFilter]);

  const sortedProcesses = [...processes].sort((a, b) => {
    const valA = a[sortField];
    const valB = b[sortField];
    if (valA === undefined || valB === undefined) return 0;
    if (valA < valB) return sortAsc ? -1 : 1;
    if (valA > valB) return sortAsc ? 1 : -1;
    return 0;
  });

  const toggleSort = (field: keyof ProcessItem) => {
    if (sortField === field) {
      setSortAsc(!sortAsc);
    } else {
      setSortField(field);
      setSortAsc(false);
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl font-bold text-white tracking-tight">Linux Process Monitor</h1>
          <p className="text-xs text-slate-400 mt-0.5">
            Active kernel task enumeration directly through /proc filesystem
          </p>
        </div>
        <div className="flex items-center space-x-3">
          <input
            type="text"
            placeholder="Search process name..."
            value={search}
            onChange={(e) => setSearch(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white placeholder-slate-500 focus:outline-none focus:border-blue-500 w-48 sm:w-64"
          />
          <select
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All States</option>
            <option value="RUNNING">RUNNING</option>
            <option value="SLEEPING">SLEEPING</option>
            <option value="STOPPED">STOPPED</option>
            <option value="CRASHED">CRASHED</option>
          </select>
          <button
            onClick={fetchProcesses}
            className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs border border-slate-700 transition"
          >
            Refresh
          </button>
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl">
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-xs">
            <thead>
              <tr className="bg-slate-950/80 border-b border-slate-800 text-slate-400 font-semibold uppercase tracking-wider text-[11px]">
                <th className="py-3 px-4 cursor-pointer hover:text-white" onClick={() => toggleSort('pid')}>
                  PID {sortField === 'pid' && (sortAsc ? '↑' : '↓')}
                </th>
                <th className="py-3 px-4 cursor-pointer hover:text-white" onClick={() => toggleSort('process_name')}>
                  Process Name {sortField === 'process_name' && (sortAsc ? '↑' : '↓')}
                </th>
                <th className="py-3 px-4">PPID</th>
                <th className="py-3 px-4 cursor-pointer hover:text-white" onClick={() => toggleSort('cpu_usage')}>
                  CPU % {sortField === 'cpu_usage' && (sortAsc ? '↑' : '↓')}
                </th>
                <th className="py-3 px-4 cursor-pointer hover:text-white" onClick={() => toggleSort('memory_usage')}>
                  Memory (MB) {sortField === 'memory_usage' && (sortAsc ? '↑' : '↓')}
                </th>
                <th className="py-3 px-4">Threads</th>
                <th className="py-3 px-4">State</th>
                <th className="py-3 px-4">Priority</th>
                <th className="py-3 px-4">Status</th>
                <th className="py-3 px-4 text-right">Action</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-mono">
              {sortedProcesses.length === 0 ? (
                <tr>
                  <td colSpan={10} className="py-8 text-center text-slate-500 font-sans">
                    {loading ? 'Scanning processes...' : 'No matching processes found.'}
                  </td>
                </tr>
              ) : (
                sortedProcesses.map((p) => (
                  <tr
                    key={p.pid}
                    className="hover:bg-slate-800/40 transition-colors group cursor-pointer"
                  >
                    <td className="py-3 px-4 font-bold text-white">{p.pid}</td>
                    <td className="py-3 px-4 font-sans font-medium text-slate-200">
                      <div className="flex items-center space-x-2">
                        <span>{p.process_name}</span>
                        {p.is_registered && (
                          <span className="text-[10px] px-1.5 py-0.5 rounded bg-blue-950 text-blue-400 border border-blue-800 font-mono">
                            REGISTERED
                          </span>
                        )}
                      </div>
                    </td>
                    <td className="py-3 px-4 text-slate-400">{p.ppid}</td>
                    <td className="py-3 px-4">
                      <span className={p.cpu_usage > 85 ? 'text-rose-400 font-bold' : 'text-slate-300'}>
                        {p.cpu_usage}%
                      </span>
                    </td>
                    <td className="py-3 px-4 text-slate-300">{p.memory_usage}</td>
                    <td className="py-3 px-4 text-slate-400">{p.thread_count}</td>
                    <td className="py-3 px-4">
                      <span className="px-2 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700 text-[10px]">
                        {p.state}
                      </span>
                    </td>
                    <td className="py-3 px-4 text-slate-400">{p.priority}</td>
                    <td className="py-3 px-4 font-sans">
                      <StatusBadge status={p.status} />
                    </td>
                    <td className="py-3 px-4 text-right font-sans">
                      <Link
                        to={`/processes/${p.pid}`}
                        className="px-2.5 py-1 rounded bg-slate-800 hover:bg-blue-600 hover:text-white text-slate-300 text-xs transition border border-slate-700"
                      >
                        Inspect
                      </Link>
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
