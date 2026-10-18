# OScope: Frontend Architecture & User Interface Design

The OScope user interface is built as a Single Page Application (SPA) utilizing **React 18**, **TypeScript**, **Vite**, **Tailwind CSS**, and **Recharts**.

It is engineered for real-time observability, clean systems-monitoring typography, high contrast dark theme aesthetics, and zero mock data.

---

## 1. Directory Structure & Architecture

```
frontend/
├── src/
│   ├── components/            # Reusable UI widgets
│   │   ├── Navbar.tsx         # Top bar with health beacon & metrics
│   │   ├── Sidebar.tsx        # Collapsible 11-route navigation panel
│   │   ├── MetricCard.tsx     # Stat cards with trend indicators
│   │   ├── StatusBadge.tsx    # Semantic color-coded pill tags
│   │   ├── IncidentTimeline.tsx # Multi-step incident lifecycle visualizer
│   │   ├── WaitForGraphViewer.tsx # SVG canvas for WFG deadlocks
│   │   └── Modal.tsx          # Accessible modal dialogs
│   ├── layouts/
│   │   └── MainLayout.tsx     # Shell containing Sidebar + Navbar + Content
│   ├── pages/                 # 11 Dedicated views
│   │   ├── DashboardPage.tsx
│   │   ├── ProcessesPage.tsx
│   │   ├── ProcessDetailPage.tsx
│   │   ├── IncidentsPage.tsx
│   │   ├── IncidentDetailPage.tsx
│   │   ├── RecoveryPage.tsx
│   │   ├── DeadlockPage.tsx
│   │   ├── PoliciesPage.tsx
│   │   ├── ResourcesPage.tsx
│   │   ├── EventsPage.tsx
│   │   └── WorkloadsPage.tsx
│   ├── services/
│   │   └── api.ts             # Axios/fetch client communicating with backend
│   ├── types/
│   │   └── index.ts           # Strict TypeScript interfaces matching backend schemas
│   ├── App.tsx                # Client-side router & route definitions
│   └── main.tsx               # Root entrypoint
├── package.json
├── tailwind.config.js
└── vite.config.ts
```

---

## 2. Page Directory & Functionality

### 1. Main Dashboard (`/`)
- System Health indicator card (`HEALTHY`, `DEGRADED`, `CRITICAL`).
- Key metrics: CPU Usage, Memory Usage, Active Processes, Active Incidents, Recovery Success Rate.
- Interactive charts: CPU & Memory telemetry over time, Incident frequency distribution, Recovery outcome breakdown.

### 2. Process Monitor (`/processes`)
- Real-time tabular process explorer: PID, Process Name, PPID, CPU %, RSS MB, Threads, State, Nice, Start Time.
- Instant search filter by name or PID, state filtering (`R`, `S`, `D`, `Z`), sortable column headers.
- Quick action buttons: View Details, Change Priority, Graceful Terminate, Force Kill.

### 3. Process Details (`/processes/:pid`)
- Complete metadata overview: Executable path, user owner, parent process.
- 5 Historical time-series charts: CPU utilization, RSS memory footprint, thread counts, context switches, page faults.
- Linked incident and recovery logs specifically associated with this PID.

### 4. Incident Center (`/incidents`)
- Incident list table: ID, Process PID/Name, Incident Type, Severity badge, Detection timestamp, Recovery Status.
- Multi-dimensional filters: Type, Severity (`CRITICAL`, `HIGH`, `MEDIUM`, `LOW`), Status (`DETECTED`, `IN_RECOVERY`, `RESOLVED`, `FAILED`).

### 5. Incident Lifecycle & Timeline (`/incidents/:id`)
- Visual chronological stage tracker:
  $$\text{Detected} \longrightarrow \text{DB Record} \longrightarrow \text{Policy Matched} \longrightarrow \text{Action Executed} \longrightarrow \text{Verification} \longrightarrow \text{Resolution}$$
- Displays audit log of POSIX actions dispatched, verification telemetry before/after, and operator notes.

### 6. Recovery Center (`/recovery`)
- Historical recovery table: Recovery ID, Incident ID, Action dispatched, Attempt count, Duration (ms), Outcome.
- Analytical charts: Success vs. Failure rates, MTTR distribution, Retry counts per incident type.

### 7. Deadlock Visualization (`/deadlocks`)
- Visual Wait-For Graph (WFG) rendered as an interactive node-link SVG canvas.
- Cycle highlighting in red with directional waiting arrows.
- Strategy switcher: `LEAST_COST`, `LOWEST_PRIORITY`, `HIGHEST_RESOURCE`.
- One-click trigger for automated victim resolution.

### 8. Policies Management (`/policies`)
- Policy rule table: Incident Type, Threshold, Persistence Count ($N$), Maximum Attempts, SIGTERM Timeout, Action.
- Modal editor allowing authorized operators to modify thresholds and toggle policies without shell injection risk.

### 9. Resource Analytics (`/resources`)
- Deep dive telemetry views: CPU core breakdown, Physical Memory vs Virtual Swap, Disk I/O bytes read/written, Total Open File Descriptors.

### 10. System Events (`/events`)
- Timeline feed of node-wide resource alerts, daemon lifecycle milestones, and threshold crossing events.

### 11. Test Workload Control Center (`/workloads`)
- Control matrix for the 9 built-in C++ fault injection binaries:
  - `cpu_hog`, `memory_hog`, `crash_test`, `crash_loop`, `hang_test`, `deadlock_test`, `thread_hog`, `io_hog`, `fd_hog`.
- One-click workload spawn and kill buttons with real-time feedback.
