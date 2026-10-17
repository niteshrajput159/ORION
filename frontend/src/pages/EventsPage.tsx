import React, { useEffect, useState } from 'react';
import { api } from '../services/api';
import { ResourceEventItem } from '../types';
import { StatusBadge } from '../components/StatusBadge';

export const EventsPage: React.FC = () => {
  const [events, setEvents] = useState<ResourceEventItem[]>([]);
  const [resourceFilter, setResourceFilter] = useState<string>('');
  const [severityFilter, setSeverityFilter] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(true);

  const fetchEvents = async () => {
    try {
      setLoading(true);
      const data = await api.getEvents(resourceFilter, severityFilter);
      setEvents(data);
    } catch (err) {
      console.error('Failed to load events:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchEvents();
    const interval = setInterval(fetchEvents, 4000);
    return () => clearInterval(interval);
  }, [resourceFilter, severityFilter]);

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-xl font-bold text-white tracking-tight">System Resource Events</h1>
          <p className="text-xs text-slate-400 mt-0.5">
            Granular threshold crossings and anomalies emitted by Member 2 resource observers
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <select
            value={resourceFilter}
            onChange={(e) => setResourceFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All Resources</option>
            <option value="CPU">CPU</option>
            <option value="MEMORY">MEMORY</option>
            <option value="SWAP">SWAP</option>
            <option value="THREADS">THREADS</option>
            <option value="FILE_DESCRIPTORS">FILE_DESCRIPTORS</option>
          </select>

          <select
            value={severityFilter}
            onChange={(e) => setSeverityFilter(e.target.value)}
            className="px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800 text-xs text-white focus:outline-none focus:border-blue-500"
          >
            <option value="">All Severities</option>
            <option value="CRITICAL">CRITICAL</option>
            <option value="WARNING">WARNING</option>
            <option value="INFO">INFO</option>
          </select>
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl">
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-xs">
            <thead>
              <tr className="bg-slate-950/80 border-b border-slate-800 text-slate-400 font-semibold uppercase tracking-wider text-[11px]">
                <th className="py-3 px-4">Event ID</th>
                <th className="py-3 px-4">Process</th>
                <th className="py-3 px-4">Resource</th>
                <th className="py-3 px-4">Event Type</th>
                <th className="py-3 px-4">Value</th>
                <th className="py-3 px-4">Threshold</th>
                <th className="py-3 px-4">Severity</th>
                <th className="py-3 px-4">Timestamp</th>
                <th className="py-3 px-4">Message</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-mono">
              {events.length === 0 ? (
                <tr>
                  <td colSpan={9} className="py-8 text-center text-slate-500 font-sans">
                    {loading ? 'Fetching resource events...' : 'No resource violation events recorded.'}
                  </td>
                </tr>
              ) : (
                events.map((ev) => (
                  <tr key={ev.event_id} className="hover:bg-slate-800/40 transition">
                    <td className="py-3 px-4 font-bold text-white">#{ev.event_id}</td>
                    <td className="py-3 px-4 font-sans font-medium text-slate-200">
                      {ev.process_name} (PID {ev.pid || '?'})
                    </td>
                    <td className="py-3 px-4 font-bold text-blue-400">{ev.resource_type}</td>
                    <td className="py-3 px-4 text-slate-300">{ev.event_type}</td>
                    <td className="py-3 px-4 font-bold text-rose-400">{ev.value}</td>
                    <td className="py-3 px-4 text-slate-400">{ev.threshold}</td>
                    <td className="py-3 px-4 font-sans">
                      <StatusBadge status={ev.severity} />
                    </td>
                    <td className="py-3 px-4 text-slate-400 text-[11px]">
                      {new Date(ev.timestamp).toLocaleTimeString()}
                    </td>
                    <td className="py-3 px-4 font-sans text-slate-300 text-[11px] max-w-xs truncate">
                      {ev.message || '--'}
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
