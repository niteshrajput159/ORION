import React from 'react';
import { StatusBadge } from './StatusBadge';
import { SystemHealth } from '../types';

interface Props {
  health: SystemHealth | null;
  onRefresh: () => void;
  isRefreshing: boolean;
}

export const Navbar: React.FC<Props> = ({ health, onRefresh, isRefreshing }) => {
  return (
    <header className="h-16 bg-slate-900 border-b border-slate-800 flex items-center justify-between px-6 sticky top-0 z-30">
      <div className="flex items-center space-x-3">
        <div className="w-9 h-9 rounded-lg bg-blue-600 flex items-center justify-center shadow-md shadow-blue-500/20">
          <svg className="w-5 h-5 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" />
          </svg>
        </div>
        <div>
          <div className="flex items-center space-x-2">
            <span className="font-bold text-lg text-white tracking-wide">Orion</span>
            <span className="text-xs px-2 py-0.5 rounded bg-blue-950 text-blue-400 border border-blue-800 font-mono">v1.0</span>
          </div>
          <p className="text-[10px] text-slate-400 font-medium">Intelligent OS Behavior & Recovery Platform</p>
        </div>
      </div>

      <div className="flex items-center space-x-4">
        {/* System Health Status Indicator */}
        <div className="flex items-center space-x-2 bg-slate-950/80 px-3 py-1.5 rounded-lg border border-slate-800">
          <span className="text-xs text-slate-400">System State:</span>
          {health ? (
            <StatusBadge status={health.health} />
          ) : (
            <span className="text-xs text-slate-500">Connecting...</span>
          )}
        </div>

        {/* Manual Refresh Button */}
        <button
          onClick={onRefresh}
          disabled={isRefreshing}
          className="p-2 rounded-lg bg-slate-800 text-slate-300 hover:bg-slate-700 hover:text-white transition border border-slate-700 disabled:opacity-50 text-xs flex items-center space-x-1"
          title="Refresh Telemetry"
        >
          <svg
            className={`w-4 h-4 ${isRefreshing ? 'animate-spin' : ''}`}
            fill="none"
            viewBox="0 0 24 24"
            stroke="currentColor"
          >
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
          </svg>
          <span className="hidden sm:inline">Refresh</span>
        </button>
      </div>
    </header>
  );
};
