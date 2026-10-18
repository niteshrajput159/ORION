import React from 'react';

interface Props {
  status: string;
  type?: 'status' | 'severity' | 'health' | 'action';
}

export const StatusBadge: React.FC<Props> = ({ status, type = 'status' }) => {
  let color = 'bg-slate-800 text-slate-300 border-slate-700';
  const s = status.toUpperCase();

  if (s === 'HEALTHY' || s === 'RESOLVED' || s === 'SUCCESS' || s === 'VERIFIED_SUCCESS' || s === 'RUNNING') {
    color = 'bg-emerald-950/70 text-emerald-400 border-emerald-700/60';
  } else if (s === 'WARNING' || s === 'MEDIUM' || s === 'INVESTIGATING' || s === 'RECOVERING' || s === 'SLEEPING') {
    color = 'bg-amber-950/70 text-amber-400 border-amber-700/60';
  } else if (s === 'CRITICAL' || s === 'HIGH' || s === 'FAILED' || s === 'VERIFIED_FAILURE' || s === 'ESCALATED' || s === 'CRASHED') {
    color = 'bg-rose-950/70 text-rose-400 border-rose-700/60 animate-pulse';
  } else if (s === 'LOW' || s === 'INFO' || s === 'OPEN') {
    color = 'bg-blue-950/70 text-blue-400 border-blue-700/60';
  }

  return (
    <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium border ${color}`}>
      <span className="w-1.5 h-1.5 mr-1.5 rounded-full bg-current opacity-80" />
      {status}
    </span>
  );
};
