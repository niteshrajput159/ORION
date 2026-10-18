import React from 'react';
import { DeadlockReport } from '../types';

interface Props {
  report: DeadlockReport;
}

export const WaitForGraphViewer: React.FC<Props> = ({ report }) => {
  const { threads, edges, cycle, victim_thread, has_deadlock } = report;

  // Compute node coordinates in a circular arrangement
  const width = 600;
  const height = 360;
  const centerX = width / 2;
  const centerY = height / 2;
  const radius = 130;

  const displayThreads = threads.length > 0 ? threads : [1, 2];

  const nodePositions: Record<number, { x: number; y: number }> = {};
  displayThreads.forEach((tid, idx) => {
    const angle = (idx / displayThreads.length) * 2 * Math.PI - Math.PI / 2;
    nodePositions[tid] = {
      x: centerX + radius * Math.cos(angle),
      y: centerY + radius * Math.sin(angle),
    };
  });

  // Check if an edge is part of the detected cycle
  const isCycleEdge = (u: number, v: number) => {
    if (!cycle || cycle.length < 2) return false;
    for (let i = 0; i < cycle.length - 1; ++i) {
      if (cycle[i] === u && cycle[i + 1] === v) return true;
    }
    return false;
  };

  return (
    <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 shadow-xl relative overflow-hidden">
      <div className="flex items-center justify-between mb-4">
        <div>
          <h3 className="text-base font-bold text-white flex items-center space-x-2">
            <span>Wait-For Graph (WFG) Topology</span>
            {has_deadlock ? (
              <span className="px-2 py-0.5 rounded text-xs bg-rose-950 text-rose-400 border border-rose-800 animate-pulse">
                CYCLE DETECTED
              </span>
            ) : (
              <span className="px-2 py-0.5 rounded text-xs bg-emerald-950 text-emerald-400 border border-emerald-800">
                ACYCLIC / HEALTHY
              </span>
            )}
          </h3>
          <p className="text-xs text-slate-400 mt-0.5">
            Directed edge T_i → T_j indicates Thread T_i is waiting for a resource held by T_j
          </p>
        </div>
      </div>

      <div className="flex justify-center items-center py-2">
        <svg width={width} height={height} className="overflow-visible select-none">
          <defs>
            <marker
              id="arrow-normal"
              viewBox="0 0 10 10"
              refX="22"
              refY="5"
              markerWidth="6"
              markerHeight="6"
              orient="auto-start-reverse"
            >
              <path d="M 0 0 L 10 5 L 0 10 z" fill="#64748b" />
            </marker>
            <marker
              id="arrow-cycle"
              viewBox="0 0 10 10"
              refX="22"
              refY="5"
              markerWidth="7"
              markerHeight="7"
              orient="auto-start-reverse"
            >
              <path d="M 0 0 L 10 5 L 0 10 z" fill="#f43f5e" />
            </marker>
          </defs>

          {/* Render directed edges */}
          {edges.map((edge, idx) => {
            const from = nodePositions[edge.waiting_thread];
            const to = nodePositions[edge.holding_thread];
            if (!from || !to) return null;

            const inCycle = isCycleEdge(edge.waiting_thread, edge.holding_thread);
            const strokeColor = inCycle ? '#f43f5e' : '#64748b';
            const markerId = inCycle ? 'url(#arrow-cycle)' : 'url(#arrow-normal)';
            const midX = (from.x + to.x) / 2;
            const midY = (from.y + to.y) / 2;

            return (
              <g key={idx} className="transition-all">
                <line
                  x1={from.x}
                  y1={from.y}
                  x2={to.x}
                  y2={to.y}
                  stroke={strokeColor}
                  strokeWidth={inCycle ? '3' : '1.5'}
                  strokeDasharray={inCycle ? 'none' : '4,4'}
                  markerEnd={markerId}
                />
                {/* Edge resource label */}
                <rect
                  x={midX - 35}
                  y={midY - 10}
                  width="70"
                  height="20"
                  rx="4"
                  fill="#0f172a"
                  stroke={strokeColor}
                  strokeWidth="1"
                />
                <text
                  x={midX}
                  y={midY + 4}
                  textAnchor="middle"
                  fill={inCycle ? '#fda4af' : '#94a3b8'}
                  fontSize="10"
                  fontFamily="monospace"
                >
                  {edge.resource}
                </text>
              </g>
            );
          })}

          {/* Render Thread Nodes */}
          {displayThreads.map((tid) => {
            const pos = nodePositions[tid];
            if (!pos) return null;
            const isVictim = tid === victim_thread;
            const inCycle = cycle && cycle.includes(tid);

            let nodeFill = '#1e293b';
            let strokeColor = '#475569';
            if (isVictim) {
              nodeFill = '#881337';
              strokeColor = '#f43f5e';
            } else if (inCycle) {
              nodeFill = '#4c0519';
              strokeColor = '#e11d48';
            }

            return (
              <g key={tid} transform={`translate(${pos.x}, ${pos.y})`} className="cursor-pointer">
                {isVictim && (
                  <circle r="34" fill="none" stroke="#f43f5e" strokeWidth="2" strokeDasharray="3,3" className="animate-spin" />
                )}
                <circle
                  r="26"
                  fill={nodeFill}
                  stroke={strokeColor}
                  strokeWidth="2.5"
                  className="shadow-lg transition-all"
                />
                <text
                  textAnchor="middle"
                  dy="-2"
                  fill="#ffffff"
                  fontSize="13"
                  fontWeight="bold"
                  fontFamily="sans-serif"
                >
                  T{tid}
                </text>
                <text
                  textAnchor="middle"
                  dy="12"
                  fill={isVictim ? '#fda4af' : '#94a3b8'}
                  fontSize="9"
                  fontFamily="monospace"
                >
                  {isVictim ? 'VICTIM' : 'Thread'}
                </text>
              </g>
            );
          })}
        </svg>
      </div>

      {has_deadlock && (
        <div className="mt-4 p-4 rounded-lg bg-rose-950/40 border border-rose-800/60 flex items-center justify-between text-xs">
          <div>
            <span className="font-bold text-rose-400">Detected Cycle: </span>
            <span className="font-mono text-white">
              {cycle.map((t, i) => `T${t}${i < cycle.length - 1 ? ' → ' : ''}`)}
            </span>
          </div>
          <div>
            <span className="font-bold text-amber-400">Victim Thread: </span>
            <span className="font-mono text-white font-bold">T{victim_thread}</span>
            <span className="text-slate-400 ml-2">({report.strategy} strategy)</span>
          </div>
        </div>
      )}
    </div>
  );
};
