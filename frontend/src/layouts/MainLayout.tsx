import React, { useState, useEffect } from 'react';
import { Outlet } from 'react-router-dom';
import { Navbar } from '../components/Navbar';
import { Sidebar } from '../components/Sidebar';
import { api } from '../services/api';
import { SystemHealth } from '../types';

export const MainLayout: React.FC = () => {
  const [health, setHealth] = useState<SystemHealth | null>(null);
  const [isRefreshing, setIsRefreshing] = useState<boolean>(false);

  const fetchHealth = async () => {
    setIsRefreshing(true);
    try {
      const data = await api.getSystemHealth();
      setHealth(data);
    } catch (err) {
      console.error('Failed to fetch system health:', err);
    } finally {
      setIsRefreshing(false);
    }
  };

  useEffect(() => {
    fetchHealth();
    // Poll system health every 3 seconds
    const interval = setInterval(fetchHealth, 3000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="flex flex-col h-screen overflow-hidden bg-slate-950 text-slate-100">
      <Navbar health={health} onRefresh={fetchHealth} isRefreshing={isRefreshing} />
      <div className="flex flex-1 overflow-hidden">
        <Sidebar />
        <main className="flex-1 overflow-y-auto p-6 bg-slate-950/60">
          <Outlet context={{ health, refreshHealth: fetchHealth }} />
        </main>
      </div>
    </div>
  );
};
