import React from 'react';
import { TimelineStep } from '../types';

interface Props {
  steps: TimelineStep[];
}

export const IncidentTimeline: React.FC<Props> = ({ steps }) => {
  return (
    <div className="flow-root py-4">
      <ul className="-mb-8">
        {steps.map((step, idx) => {
          const isLast = idx === steps.length - 1;

          let iconBg = 'bg-slate-700';
          let iconBorder = 'border-slate-600';
          let textColor = 'text-slate-300';

          if (step.status === 'COMPLETED') {
            iconBg = 'bg-emerald-600';
            iconBorder = 'border-emerald-500';
            textColor = 'text-emerald-400';
          } else if (step.status === 'FAILED') {
            iconBg = 'bg-rose-600';
            iconBorder = 'border-rose-500';
            textColor = 'text-rose-400';
          } else if (step.status === 'IN_PROGRESS') {
            iconBg = 'bg-amber-600';
            iconBorder = 'border-amber-500';
            textColor = 'text-amber-400';
          }

          return (
            <li key={idx}>
              <div className="relative pb-8">
                {!isLast && (
                  <span
                    className="absolute top-4 left-4 -ml-px h-full w-0.5 bg-slate-800"
                    aria-hidden="true"
                  />
                )}
                <div className="relative flex space-x-3 items-start">
                  <div>
                    <span
                      className={`h-8 w-8 rounded-full flex items-center justify-center ring-4 ring-slate-950 ${iconBg} ${iconBorder} border`}
                    >
                      {step.status === 'COMPLETED' ? (
                        <svg className="w-4 h-4 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M5 13l4 4L19 7" />
                        </svg>
                      ) : step.status === 'FAILED' ? (
                        <svg className="w-4 h-4 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M6 18L18 6M6 6l12 12" />
                        </svg>
                      ) : step.status === 'IN_PROGRESS' ? (
                        <span className="w-2.5 h-2.5 bg-white rounded-full animate-ping" />
                      ) : (
                        <span className="w-2 h-2 bg-slate-400 rounded-full" />
                      )}
                    </span>
                  </div>
                  <div className="min-w-0 flex-1 pt-1.5 flex justify-between space-x-4">
                    <div>
                      <p className={`text-sm font-semibold ${textColor}`}>{step.step_title}</p>
                      <p className="text-xs text-slate-400 mt-0.5">{step.description}</p>
                    </div>
                    <div className="text-right text-xs whitespace-nowrap text-slate-500 font-mono">
                      {step.timestamp}
                    </div>
                  </div>
                </div>
              </div>
            </li>
          );
        })}
      </ul>
    </div>
  );
};
