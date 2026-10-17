import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { api } from '../services/api';
import { IncidentDetail } from '../types';
import { IncidentTimeline } from '../components/IncidentTimeline';
import { StatusBadge } from '../components/StatusBadge';

export const IncidentDetailPage: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const [detail, setDetail] = useState<IncidentDetail | null>(null);
  const [aiExplanation, setAiExplanation] = useState<any>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [loadingAi, setLoadingAi] = useState<boolean>(false);

  useEffect(() => {
    if (!id) return;
    const fetchDetail = async () => {
      try {
        setLoading(true);
        const data = await api.getIncidentDetail(Number(id));
        setDetail(data);
      } catch (err) {
        console.error('Failed to load incident detail:', err);
      } finally {
        setLoading(false);
      }
    };
    fetchDetail();
  }, [id]);

  const loadAiDiagnostics = async () => {
    if (!id) return;
    try {
      setLoadingAi(true);
      const res = await api.getAiExplanation(Number(id));
      setAiExplanation(res);
    } catch (err) {
      console.error('Failed to fetch AI explanation:', err);
    } finally {
      setLoadingAi(false);
    }
  };

  if (!detail && !loading) {
    return (
      <div className="text-center py-12">
        <h2 className="text-lg font-bold text-white">Incident #{id} Not Found</h2>
        <Link to="/incidents" className="mt-4 inline-block px-4 py-2 bg-blue-600 rounded-lg text-xs text-white">
          Back to Incidents
        </Link>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header card */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 shadow-xl">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
          <div>
            <div className="flex items-center space-x-3">
              <span className="font-mono text-sm px-2.5 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700">
                Incident #{detail?.incident_id}
              </span>
              <h1 className="text-2xl font-bold text-white">{detail?.incident_type}</h1>
              {detail && <StatusBadge status={detail.severity} />}
              {detail && <StatusBadge status={detail.status} />}
            </div>
            <p className="text-xs text-slate-400 mt-2">{detail?.description}</p>
          </div>

          <div className="flex items-center space-x-3">
            <button
              onClick={loadAiDiagnostics}
              disabled={loadingAi}
              className="px-3.5 py-2 rounded-lg bg-purple-950/80 hover:bg-purple-900 text-purple-300 border border-purple-800 text-xs font-medium transition flex items-center space-x-2"
            >
              <svg className="w-4 h-4 text-purple-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" />
              </svg>
              <span>{loadingAi ? 'Analyzing Telemetry...' : 'AI Root Cause Analysis'}</span>
            </button>
          </div>
        </div>

        {/* Essential Incident Fields Grid */}
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 mt-6 pt-6 border-t border-slate-800 text-xs">
          <div>
            <span className="text-slate-500 block">Target Process</span>
            <span className="font-semibold text-white mt-0.5 block">
              {detail?.process_name} (PID {detail?.pid || '?'})
            </span>
          </div>
          <div>
            <span className="text-slate-500 block">Matched Policy</span>
            <span className="font-mono text-white mt-0.5 block">{detail?.policy_name || 'DEFAULT'}</span>
          </div>
          <div>
            <span className="text-slate-500 block">Recovery Action</span>
            <span className="font-mono text-emerald-400 mt-0.5 block font-bold">{detail?.action_taken}</span>
          </div>
          <div>
            <span className="text-slate-500 block">Attempts / Verification</span>
            <span className="font-mono text-white mt-0.5 block">
              {detail?.attempts} attempt(s) • {detail?.verification_result || 'Verified'}
            </span>
          </div>
        </div>
      </div>

      {/* AI Diagnostic Explanation Box */}
      {aiExplanation && (
        <div className="bg-purple-950/30 border border-purple-800/60 rounded-xl p-6 shadow-xl animate-fadeIn">
          <div className="flex items-center space-x-2 mb-3">
            <span className="p-1 rounded bg-purple-900/60 text-purple-400">
              <svg className="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z" />
              </svg>
            </span>
            <h3 className="text-sm font-bold text-purple-200">AI Telemetry Advisory & Diagnostic Explanation</h3>
          </div>
          <div className="space-y-3 text-xs text-purple-100 leading-relaxed font-sans">
            <div>
              <span className="font-bold text-purple-300">Root Cause Hypothesis: </span>
              {aiExplanation.root_cause}
            </div>
            <div>
              <span className="font-bold text-purple-300">Architectural Recommendation: </span>
              {aiExplanation.recommendation}
            </div>
          </div>
        </div>
      )}

      {/* Chronological Complete Timeline Card */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 shadow-xl">
        <div className="pb-3 border-b border-slate-800 mb-2">
          <h2 className="text-sm font-bold text-white uppercase tracking-wider">
            Closed-Loop Execution Timeline
          </h2>
          <p className="text-xs text-slate-400 mt-0.5">
            Step-by-step trace: Observe → Detect → Decide → Act → Verify → Recover
          </p>
        </div>

        {detail?.timeline && <IncidentTimeline steps={detail.timeline} />}
      </div>
    </div>
  );
};
