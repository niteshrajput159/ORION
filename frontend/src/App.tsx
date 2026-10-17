import React from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { MainLayout } from './layouts/MainLayout';
import { DashboardPage } from './pages/DashboardPage';
import { ProcessesPage } from './pages/ProcessesPage';
import { ProcessDetailPage } from './pages/ProcessDetailPage';
import { IncidentsPage } from './pages/IncidentsPage';
import { IncidentDetailPage } from './pages/IncidentDetailPage';
import { RecoveryPage } from './pages/RecoveryPage';
import { DeadlockPage } from './pages/DeadlockPage';
import { PoliciesPage } from './pages/PoliciesPage';
import { ResourcesPage } from './pages/ResourcesPage';
import { EventsPage } from './pages/EventsPage';
import { WorkloadsPage } from './pages/WorkloadsPage';

export const App: React.FC = () => {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<MainLayout />}>
          <Route index element={<DashboardPage />} />
          <Route path="processes" element={<ProcessesPage />} />
          <Route path="processes/:pid" element={<ProcessDetailPage />} />
          <Route path="incidents" element={<IncidentsPage />} />
          <Route path="incidents/:id" element={<IncidentDetailPage />} />
          <Route path="recovery" element={<RecoveryPage />} />
          <Route path="deadlocks" element={<DeadlockPage />} />
          <Route path="policies" element={<PoliciesPage />} />
          <Route path="resources" element={<ResourcesPage />} />
          <Route path="events" element={<EventsPage />} />
          <Route path="workloads" element={<WorkloadsPage />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
};

export default App;
