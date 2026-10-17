import React, { useEffect, useState } from 'react';
import { api } from '../services/api';
import { DeadlockReport } from '../types';
import { WaitForGraphViewer } from '../components/WaitForGraphViewer';
import { StatusBadge } from '../components/StatusBadge';

export const DeadlockPage: React.FC = () => {
  const [report, setReport] = useState<DeadlockReport | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [resolving, setResolving] = useState<boolean>(false);
  const [actionMessage, setActionMessage] = useState<string>('');

  const fetchDeadlock = async () => {
    try {
      setLoading(true);
      const data = await api.getDeadlockReport();
      setReport(data);
    } catch (err) {
      console.error('Failed to load deadlock report:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchDeadlock();
    const interval = setInterval(fetchDeadlock, 3000);
    return () => clearInterval(interval);
  }, []);

  const handleLaunchDeadlockWorkload = async () => {
    try {
      setActionMessage('Starting real multithreaded deadlock workload (deadlock_test)...');
      await api.startWorkload('deadlock_test');
      setActionMessage('Workload launched. Analyzing Wait-For Graph...');
      fetchDeadlock();
    } catch (err: any) {
      setActionMessage(`Error: ${err.message}`);
    }
  };

  const handleStopWorkload = async () => {
    try {
      setResolving(true);
      setActionMessage('Stopping deadlock workload...');
      await api.stopWorkload('deadlock_test');
      setActionMessage('Workload stopped. Re-analyzing Wait-For Graph...');
      fetchDeadlock();
    } catch (err: any) {
      setActionMessage(`Error: ${err.message}`);
    } finally {
      setResolving(false);
    }
  };

  const handleResolveDeadlock = async () => {
    try {
      setResolving(true);
      setActionMessage('Executing LEAST_COST preemption on victim thread to break cycle...');
      const res = await api.resolveDeadlock('LEAST_COST');
      setActionMessage(res.message || 'Deadlock resolved successfully! Wait-For Graph verified acyclic.');
      await fetchDeadlock();
    } catch (err: any) {
      setActionMessage(`Resolution error: ${err.message}`);
    } finally {
      setResolving(false);
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl font-bold text-white tracking-tight">Deadlock Analysis & Cycle Elimination</h1>
          <p className="text-xs text-slate-400 mt-0.5">
            DFS Cycle Detection over directed Wait-For Graph (WFG) with configurable victim selection
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <button
            onClick={handleLaunchDeadlockWorkload}
            className="px-3 py-1.5 rounded-lg bg-rose-600 hover:bg-rose-500 text-white text-xs font-semibold shadow-lg shadow-rose-600/30 transition flex items-center space-x-1.5"
          >
            <span>Trigger Deadlock Workload</span>
          </button>
          <button
            onClick={handleResolveDeadlock}
            disabled={resolving}
            className="px-3.5 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-lg shadow-emerald-600/30 transition flex items-center space-x-1.5"
          >
            <span>Resolve Active Deadlock</span>
          </button>
          <button
            onClick={handleStopWorkload}
            disabled={resolving}
            className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs border border-slate-700 transition"
          >
            Stop Workload
          </button>
        </div>
      </div>

      {actionMessage && (
        <div className="p-3 rounded-lg bg-slate-900 border border-slate-800 text-xs text-blue-300">
          {actionMessage}
        </div>
      )}

      {/* Deadlock State Banner */}
      {report && (
        <div className={`p-6 rounded-xl border shadow-xl ${
          report.has_deadlock
            ? 'bg-rose-950/40 border-rose-800/80'
            : 'bg-emerald-950/30 border-emerald-800/60'
        }`}>
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
            <div>
              <div className="flex items-center space-x-3">
                <span className={`w-3 h-3 rounded-full ${report.has_deadlock ? 'bg-rose-500 animate-ping' : 'bg-emerald-500'}`} />
                <h2 className="text-lg font-bold text-white">
                  {report.has_deadlock ? 'DEADLOCK DETECTED IN PROCESS TASK GRAPH' : 'NO DEADLOCK DETECTED'}
                </h2>
              </div>
              <p className="text-xs text-slate-300 mt-1">
                {report.has_deadlock
                  ? 'Circular mutex hold-and-wait dependency identified via DFS graph cycle traversal.'
                  : 'Wait-For Graph is strictly acyclic; threads are executing without circular blocking.'}
              </p>
            </div>
            <div>
              <StatusBadge status={report.has_deadlock ? 'CRITICAL' : 'HEALTHY'} />
            </div>
          </div>

          {/* 6 Essential Step Details Required by Spec */}
          <div className="grid grid-cols-2 sm:grid-cols-6 gap-4 mt-6 pt-6 border-t border-slate-800/80 text-xs">
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Cycle Detected</span>
              <span className="font-mono text-white mt-1 block font-bold">
                {report.cycle.length > 0 ? report.cycle.map((t) => `T${t}`).join(' → ') : 'None'}
              </span>
            </div>
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Victim Selected</span>
              <span className="font-mono text-rose-400 mt-1 block font-bold">
                {report.victim_thread > 0 ? `T${report.victim_thread}` : 'None'}
              </span>
            </div>
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Strategy</span>
              <span className="font-mono text-amber-300 mt-1 block font-medium">
                {report.strategy}
              </span>
            </div>
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Recovery Action</span>
              <span className="font-mono text-emerald-400 mt-1 block font-bold">
                {report.action}
              </span>
            </div>
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Verification</span>
              <span className="font-mono text-white mt-1 block">
                {report.verification}
              </span>
            </div>
            <div>
              <span className="text-slate-400 block font-semibold uppercase text-[10px]">Final Result</span>
              <span className="font-mono mt-1 block font-bold">
                <StatusBadge status={report.final_result} />
              </span>
            </div>
          </div>
        </div>
      )}

      {/* Interactive Topology Graph Viewer */}
      {report && <WaitForGraphViewer report={report} />}
    </div>
  );
};
