import React, { useEffect, useState } from 'react';
import { api } from '../services/api';
import { PolicyItem } from '../types';
import { Modal } from '../components/Modal';

const ALLOWED_ACTIONS = [
  'LOWER_PRIORITY',
  'SUSPEND',
  'RESUME',
  'RESTART',
  'TERMINATE_SIGTERM',
  'TERMINATE_SIGKILL',
  'RESOLVE_DEADLOCK',
  'ALERT_ONLY'
];

export const PoliciesPage: React.FC = () => {
  const [policies, setPolicies] = useState<PolicyItem[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [editingPolicy, setEditingPolicy] = useState<PolicyItem | null>(null);
  const [formData, setFormData] = useState<Partial<PolicyItem>>({});
  const [saving, setSaving] = useState<boolean>(false);
  const [message, setMessage] = useState<string>('');

  const fetchPolicies = async () => {
    try {
      setLoading(true);
      const data = await api.getPolicies();
      setPolicies(data);
    } catch (err) {
      console.error('Failed to load policies:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchPolicies();
  }, []);

  const openEditModal = (p: PolicyItem) => {
    setEditingPolicy(p);
    setFormData({
      cpu_threshold: p.cpu_threshold,
      memory_threshold: p.memory_threshold,
      persistence_count: p.persistence_count,
      max_attempts: p.max_attempts,
      timeout_seconds: p.timeout_seconds,
      action: p.action,
      enabled: p.enabled
    });
    setMessage('');
  };

  const handleSave = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!editingPolicy) return;
    try {
      setSaving(true);
      await api.updatePolicy(editingPolicy.policy_id, formData);
      setMessage('Policy successfully updated.');
      setEditingPolicy(null);
      fetchPolicies();
    } catch (err: any) {
      setMessage(`Failed to update policy: ${err.message}`);
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-xl font-bold text-white tracking-tight">Recovery Policy Engine</h1>
        <p className="text-xs text-slate-400 mt-0.5">
          Declarative decision rules mapping anomaly classifications to verified automated OS actions
        </p>
      </div>

      {message && (
        <div className="p-3 rounded-lg bg-slate-900 border border-blue-800 text-xs text-blue-300">
          {message}
        </div>
      )}

      <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl">
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-xs">
            <thead>
              <tr className="bg-slate-950/80 border-b border-slate-800 text-slate-400 font-semibold uppercase tracking-wider text-[11px]">
                <th className="py-3 px-4">ID</th>
                <th className="py-3 px-4">Incident Type</th>
                <th className="py-3 px-4">Severity</th>
                <th className="py-3 px-4">Action</th>
                <th className="py-3 px-4">CPU Limit</th>
                <th className="py-3 px-4">Mem Limit</th>
                <th className="py-3 px-4">Persistence</th>
                <th className="py-3 px-4">Max Attempts</th>
                <th className="py-3 px-4">Timeout</th>
                <th className="py-3 px-4">Enabled</th>
                <th className="py-3 px-4 text-right">Configure</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-mono">
              {policies.map((p) => (
                <tr key={p.policy_id} className="hover:bg-slate-800/40 transition">
                  <td className="py-3 px-4 font-bold text-white">#{p.policy_id}</td>
                  <td className="py-3 px-4 font-sans font-semibold text-white">{p.incident_type}</td>
                  <td className="py-3 px-4 text-slate-300">{p.severity}</td>
                  <td className="py-3 px-4 font-bold text-blue-400">{p.action}</td>
                  <td className="py-3 px-4 text-slate-300">{p.cpu_threshold}%</td>
                  <td className="py-3 px-4 text-slate-300">{p.memory_threshold}%</td>
                  <td className="py-3 px-4 text-slate-300">{p.persistence_count} samples</td>
                  <td className="py-3 px-4 text-slate-300">{p.max_attempts}</td>
                  <td className="py-3 px-4 text-slate-300">{p.timeout_seconds}s</td>
                  <td className="py-3 px-4">
                    <span
                      className={`px-2 py-0.5 rounded text-[10px] ${
                        p.enabled
                          ? 'bg-emerald-950 text-emerald-400 border border-emerald-800'
                          : 'bg-slate-800 text-slate-500 border border-slate-700'
                      }`}
                    >
                      {p.enabled ? 'ACTIVE' : 'DISABLED'}
                    </span>
                  </td>
                  <td className="py-3 px-4 text-right font-sans">
                    <button
                      onClick={() => openEditModal(p)}
                      className="px-2.5 py-1 rounded bg-slate-800 hover:bg-blue-600 hover:text-white text-slate-300 text-xs transition border border-slate-700"
                    >
                      Edit Rule
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Edit Policy Modal */}
      <Modal
        isOpen={!!editingPolicy}
        onClose={() => setEditingPolicy(null)}
        title={`Configure Policy #${editingPolicy?.policy_id} (${editingPolicy?.incident_type})`}
      >
        <form onSubmit={handleSave} className="space-y-4 text-xs">
          <div>
            <label className="block text-slate-300 mb-1 font-medium">Recovery Action</label>
            <select
              value={formData.action || ''}
              onChange={(e) => setFormData({ ...formData, action: e.target.value })}
              className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
            >
              {ALLOWED_ACTIONS.map((act) => (
                <option key={act} value={act}>{act}</option>
              ))}
            </select>
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="block text-slate-300 mb-1 font-medium">CPU Threshold (%)</label>
              <input
                type="number"
                min="10"
                max="100"
                value={formData.cpu_threshold ?? 85}
                onChange={(e) => setFormData({ ...formData, cpu_threshold: parseFloat(e.target.value) })}
                className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
              />
            </div>
            <div>
              <label className="block text-slate-300 mb-1 font-medium">Memory Threshold (%)</label>
              <input
                type="number"
                min="10"
                max="100"
                value={formData.memory_threshold ?? 80}
                onChange={(e) => setFormData({ ...formData, memory_threshold: parseFloat(e.target.value) })}
                className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
              />
            </div>
          </div>

          <div className="grid grid-cols-3 gap-4">
            <div>
              <label className="block text-slate-300 mb-1 font-medium">Persistence (samples)</label>
              <input
                type="number"
                min="1"
                max="20"
                value={formData.persistence_count ?? 4}
                onChange={(e) => setFormData({ ...formData, persistence_count: parseInt(e.target.value, 10) })}
                className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
              />
            </div>
            <div>
              <label className="block text-slate-300 mb-1 font-medium">Max Attempts</label>
              <input
                type="number"
                min="0"
                max="10"
                value={formData.max_attempts ?? 3}
                onChange={(e) => setFormData({ ...formData, max_attempts: parseInt(e.target.value, 10) })}
                className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
              />
            </div>
            <div>
              <label className="block text-slate-300 mb-1 font-medium">Timeout (sec)</label>
              <input
                type="number"
                min="1"
                max="60"
                value={formData.timeout_seconds ?? 5}
                onChange={(e) => setFormData({ ...formData, timeout_seconds: parseInt(e.target.value, 10) })}
                className="w-full px-3 py-2 rounded-lg bg-slate-950 border border-slate-800 text-white focus:outline-none focus:border-blue-500"
              />
            </div>
          </div>

          <div className="flex items-center space-x-2 pt-2">
            <input
              type="checkbox"
              id="enabledCheck"
              checked={formData.enabled ?? true}
              onChange={(e) => setFormData({ ...formData, enabled: e.target.checked })}
              className="rounded bg-slate-950 border-slate-800 text-blue-600 focus:ring-0"
            />
            <label htmlFor="enabledCheck" className="text-slate-300 font-medium cursor-pointer">
              Policy Rule Enabled
            </label>
          </div>

          <div className="flex justify-end space-x-3 pt-4 border-t border-slate-800">
            <button
              type="button"
              onClick={() => setEditingPolicy(null)}
              className="px-4 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 transition"
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={saving}
              className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white font-medium transition disabled:opacity-50"
            >
              {saving ? 'Saving...' : 'Save Policy Changes'}
            </button>
          </div>
        </form>
      </Modal>
    </div>
  );
};
