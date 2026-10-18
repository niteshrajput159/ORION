import React from 'react';

interface Props {
  title: string;
  value: string | number;
  subtitle?: string;
  icon?: React.ReactNode;
  trend?: string;
  variant?: 'default' | 'success' | 'warning' | 'danger';
}

export const MetricCard: React.FC<Props> = ({
  title,
  value,
  subtitle,
  icon,
  trend,
  variant = 'default'
}) => {
  let borderColor = 'border-slate-800';
  let glowColor = '';

  if (variant === 'success') {
    borderColor = 'border-emerald-800/40';
    glowColor = 'shadow-emerald-950/20';
  } else if (variant === 'warning') {
    borderColor = 'border-amber-800/40';
    glowColor = 'shadow-amber-950/20';
  } else if (variant === 'danger') {
    borderColor = 'border-rose-800/40';
    glowColor = 'shadow-rose-950/20';
  }

  return (
    <div className={`bg-slate-900/90 rounded-xl p-5 border ${borderColor} shadow-lg ${glowColor} transition-all duration-200 hover:border-slate-700`}>
      <div className="flex items-center justify-between">
        <span className="text-xs font-semibold uppercase tracking-wider text-slate-400">{title}</span>
        {icon && <div className="text-slate-400 p-2 bg-slate-800/80 rounded-lg">{icon}</div>}
      </div>
      <div className="mt-3 flex items-baseline justify-between">
        <div className="text-2xl font-bold tracking-tight text-white">{value}</div>
        {trend && <span className="text-xs text-slate-400">{trend}</span>}
      </div>
      {subtitle && <p className="mt-1 text-xs text-slate-500">{subtitle}</p>}
    </div>
  );
};
