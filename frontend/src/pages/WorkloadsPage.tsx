import React, { useEffect, useState } from 'react';
import { api } from '../services/api';

interface WorkloadMeta {
  name: string;
  title: string;
  description: string;
  faultType: string;
  expectedAction: string;
  defaultArgs?: string[];
}

const WORKLOADS: WorkloadMeta[] = [
  {
    name: 'cpu_hog',
    title: 'Multi-Core CPU Saturation',
    description: 'Runs heavy mathematical loops across worker threads to saturate CPU capacity.',
    faultType: 'HIGH_CPU',
    expectedAction: 'LOWER_PRIORITY (+10 Nice)',
    defaultArgs: ['--threads', '4', '--duration', '60']
  },
  {
    name: 'memory_hog',
    title: 'Physical RSS Memory Leak',
    description: 'Allocates chunks and touches 4KB pages to force physical memory exhaustion.',
    faultType: 'HIGH_MEMORY',
    expectedAction: 'TERMINATE_SIGTERM',
    defaultArgs: ['--target-mb', '300', '--rate-mb', '50']
  },
  {
    name: 'crash_test',
    title: 'Unexpected Fatal Crash',
    description: 'Simulates abnormal null pointer dereference after a 2-second delay.',
    faultType: 'PROCESS_CRASH',
    expectedAction: 'RESTART & VERIFY',
    defaultArgs: ['--delay', '2', '--mode', 'segfault']
  },
  {
    name: 'crash_loop',
    title: 'Crash Loop Thrashing',
    description: 'Repeatedly crashes upon restart. Tests bounded retries (stops at 3 attempts).',
    faultType: 'CRASH_LOOP',
    expectedAction: 'BOUNDED ESCALATION'
  },
  {
    name: 'hang_test',
    title: 'Unresponsive Hung Process',
    description: 'Ignores graceful SIGTERM signals, testing staged escalation to uncatchable SIGKILL.',
    faultType: 'HUNG_PROCESS',
    expectedAction: 'ESCALATE SIGKILL'
  },
  {
    name: 'deadlock_test',
    title: 'Multithreaded Deadlock Cycle',
    description: 'Spawns 2 threads with circular mutex dependencies (T1 -> T2 and T2 -> T1).',
    faultType: 'DEADLOCK',
    expectedAction: 'VICTIM TERMINATION'
  },
  {
    name: 'thread_hog',
    title: 'Thread Pool Exhaustion',
    description: 'Spawns 80 active POSIX threads exceeding the 50-thread threshold.',
    faultType: 'EXCESSIVE_THREADS',
    expectedAction: 'TERMINATE_SIGTERM'
  },
  {
    name: 'io_hog',
    title: 'Synchronized Disk I/O Saturation',
    description: 'Performs synchronized fsync writes to disk to exceed I/O rate limits.',
    faultType: 'HIGH_IO',
    expectedAction: 'LOWER_PRIORITY'
  },
  {
    name: 'fd_hog',
    title: 'File Descriptor Exhaustion',
    description: 'Opens 600 pipes exceeding the 500 file descriptor quota.',
    faultType: 'EXCESSIVE_FILE_DESCRIPTORS',
    expectedAction: 'TERMINATE_SIGTERM'
  }
];

export const WorkloadsPage: React.FC = () => {
  const [statuses, setStatuses] = useState<Record<string, { running: boolean; pid: number | null }>>({});
  const [actionMsg, setActionMsg] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(true);

  const fetchStatuses = async () => {
    try {
      const data = await api.getWorkloads();
      setStatuses(data);
    } catch (err) {
      console.error('Failed to load workload statuses:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchStatuses();
    const interval = setInterval(fetchStatuses, 3000);
    return () => clearInterval(interval);
  }, []);

  const handleStart = async (wl: WorkloadMeta) => {
    try {
      setActionMsg(`Starting ${wl.name}...`);
      const res = await api.startWorkload(wl.name, wl.defaultArgs);
      setActionMsg(res.message || `Started ${wl.name}`);
      fetchStatuses();
    } catch (err: any) {
      setActionMsg(`Error launching ${wl.name}: ${err.message}`);
    }
  };

  const handleStop = async (name: string) => {
    try {
      setActionMsg(`Stopping ${name}...`);
      const res = await api.stopWorkload(name);
      setActionMsg(res.message || `Stopped ${name}`);
      fetchStatuses();
    } catch (err: any) {
      setActionMsg(`Error stopping ${name}: ${err.message}`);
    }
  };

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-xl font-bold text-white tracking-tight">Fault-Injection Test Suite & Demonstration Center</h1>
        <p className="text-xs text-slate-400 mt-0.5">
          Execute real controlled POSIX stress workloads to verify closed-loop Orion observation, detection, and recovery
        </p>
      </div>

      {actionMsg && (
        <div className="p-3 rounded-lg bg-slate-900 border border-blue-800 text-xs text-blue-300">
          {actionMsg}
        </div>
      )}

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {WORKLOADS.map((wl) => {
          const status = statuses[wl.name];
          const isRunning = status?.running || false;
          const pid = status?.pid;

          return (
            <div
              key={wl.name}
              className={`rounded-xl p-5 border shadow-xl flex flex-col justify-between transition-all ${
                isRunning
                  ? 'bg-blue-950/20 border-blue-800/80'
                  : 'bg-slate-900 border-slate-800 hover:border-slate-700'
              }`}
            >
              <div>
                <div className="flex items-center justify-between">
                  <h3 className="text-sm font-bold text-white">{wl.title}</h3>
                  {isRunning ? (
                    <span className="px-2 py-0.5 rounded-full text-[10px] bg-emerald-950 text-emerald-400 border border-emerald-800 animate-pulse font-mono">
                      PID {pid}
                    </span>
                  ) : (
                    <span className="px-2 py-0.5 rounded-full text-[10px] bg-slate-800 text-slate-400 border border-slate-700 font-mono">
                      IDLE
                    </span>
                  )}
                </div>

                <p className="text-xs text-slate-400 mt-2 leading-relaxed">
                  {wl.description}
                </p>

                <div className="mt-4 pt-3 border-t border-slate-800/80 space-y-1 text-[11px] font-mono">
                  <div className="flex justify-between">
                    <span className="text-slate-500">Anomaly:</span>
                    <span className="text-amber-400">{wl.faultType}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-500">Expected Action:</span>
                    <span className="text-blue-400">{wl.expectedAction}</span>
                  </div>
                </div>
              </div>

              <div className="mt-5 pt-3 border-t border-slate-800/80 flex items-center justify-end space-x-2">
                {isRunning ? (
                  <div className="w-full flex items-center space-x-2">
                    <button
                      onClick={async () => {
                        try {
                          const res = await api.remediateWorkload(wl.name);
                          setActionMsg(res.message || `Automated recovery executed for ${wl.name}.`);
                          fetchStatuses();
                        } catch (e: any) {
                          setActionMsg(`Recovery error: ${e.message}`);
                        }
                      }}
                      className="flex-1 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white font-medium text-xs transition shadow-md shadow-emerald-600/20"
                      title="Trigger automated policy remediation & verification"
                    >
                      Auto-Recover
                    </button>
                    <button
                      onClick={() => handleStop(wl.name)}
                      className="py-1.5 px-3 rounded-lg bg-rose-600 hover:bg-rose-500 text-white font-medium text-xs transition"
                    >
                      Stop
                    </button>
                  </div>
                ) : (
                  <button
                    onClick={() => handleStart(wl)}
                    className="w-full py-1.5 rounded-lg bg-blue-600 hover:bg-blue-500 text-white font-medium text-xs transition shadow-md shadow-blue-600/20"
                  >
                    Launch Workload
                  </button>
                )}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
