# OScope: Backend Service Architecture & REST API

The OScope Backend is implemented using **FastAPI** and **SQLAlchemy 2.0**, serving as the central orchestration hub between the C++ Core Engine, the PostgreSQL database, and the React frontend.

---

## 1. Architectural Design & Security Safeguards

### Architectural Principles:
1. **Asynchronous Non-Blocking REST API**: Built on ASGI (`uvicorn`) with FastAPI routers.
2. **Database Resilience**: Connects to PostgreSQL by default via connection pooling. Includes an automatic SQLite local fallback (`sqlite:///./oscope.db`) for isolated environments and offline unit tests.
3. **IPC Client**: Maintains a resilient Unix Domain Socket connection (`/tmp/oscope.sock`) to the C++ core daemon.
4. **Zero Arbitrary Command Execution**: The backend strictly whitelists permitted operational commands:
   - `CHANGE_PRIORITY`
   - `TERMINATE_SIGTERM`
   - `TERMINATE_SIGKILL`
   - `SUSPEND_PROCESS`
   - `RESUME_PROCESS`
   - `RESTART_PROCESS`
   - `RESOLVE_DEADLOCK`
   Any command outside this whitelist is rejected immediately with HTTP 400.

---

## 2. Directory Layout

```
backend/
├── app/
│   ├── api/                   # FastAPI APIRouters
│   │   ├── system.py          # /api/system
│   │   ├── processes.py       # /api/processes
│   │   ├── incidents.py       # /api/incidents
│   │   ├── recovery.py        # /api/recovery
│   │   ├── resources.py       # /api/resources
│   │   ├── events.py          # /api/events
│   │   ├── policies.py        # /api/policies
│   │   ├── deadlocks.py       # /api/deadlocks
│   │   ├── analytics.py       # /api/analytics
│   │   └── workloads.py       # /api/workloads
│   ├── models/                # SQLAlchemy ORM Models
│   │   ├── process.py
│   │   ├── snapshot.py
│   │   ├── incident.py
│   │   ├── policy.py
│   │   ├── action_log.py
│   │   ├── recovery.py
│   │   └── resource_event.py
│   ├── schemas/               # Pydantic Schemas (Request/Response)
│   ├── services/              # Business Logic & Core Client
│   │   ├── core_client.py     # IPC Socket client
│   │   ├── system_service.py
│   │   ├── process_service.py
│   │   ├── incident_service.py
│   │   ├── recovery_service.py
│   │   ├── policy_service.py
│   │   ├── workload_service.py
│   │   └── ai_service.py      # Diagnostic insights & explanation
│   ├── config.py              # Environment configuration loader
│   ├── database.py            # SQLAlchemy engine, sessionmaker & init
│   └── main.py                # FastAPI app initialization & CORS
├── tests/
│   ├── test_api.py            # Endpoint route verification
│   └── test_services.py       # Service logic verification
└── requirements.txt
```

---

## 3. Core REST API Catalog

| HTTP Method | Route | Description |
|---|---|---|
| `GET` | `/healthz` | Backend liveness probe |
| `GET` | `/api/system/health` | Overall system health, CPU/RAM utilization |
| `GET` | `/api/processes` | List active processes with CPU, memory, thread metrics |
| `GET` | `/api/processes/{pid}` | Process detail view with historical snapshots |
| `POST` | `/api/processes/{pid}/action` | Trigger controlled action (`SIGTERM`, `SIGKILL`, `NICE`) |
| `GET` | `/api/incidents` | List detected incidents with filter parameters |
| `GET` | `/api/incidents/{id}` | Detailed incident timeline and audit logs |
| `GET` | `/api/recovery` | List recovery actions and outcomes |
| `GET` | `/api/deadlocks` | Get current Wait-For Graph (WFG) and detected cycles |
| `POST` | `/api/deadlocks/resolve` | Trigger cycle-breaking victim selection and preemption |
| `GET` | `/api/policies` | Retrieve active automated remediation policies |
| `PUT` | `/api/policies/{id}` | Update policy thresholds or toggle status |
| `GET` | `/api/resources/current` | Real-time aggregate resource metrics |
| `GET` | `/api/resources/history` | Time-series system telemetry for dashboard charts |
| `GET` | `/api/events` | List system resource events |
| `GET` | `/api/analytics/summary` | MTTD, MTTR, success rates, incident distributions |
| `GET` | `/api/workloads` | List status of 9 fault-injection binaries |
| `POST` | `/api/workloads/{name}/start` | Spawn test workload binary |
| `POST` | `/api/workloads/{name}/stop` | Stop test workload binary |
| `GET` | `/api/ai/diagnose/{incident_id}` | Generates explainable root-cause diagnosis |
