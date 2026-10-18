from app.api.system import router as system_router
from app.api.processes import router as processes_router
from app.api.incidents import router as incidents_router
from app.api.recovery import router as recovery_router
from app.api.resources import router as resources_router
from app.api.events import router as events_router
from app.api.policies import router as policies_router
from app.api.deadlocks import router as deadlocks_router
from app.api.analytics import router as analytics_router
from app.api.workloads import router as workloads_router

__all__ = [
    "system_router",
    "processes_router",
    "incidents_router",
    "recovery_router",
    "resources_router",
    "events_router",
    "policies_router",
    "deadlocks_router",
    "analytics_router",
    "workloads_router",
]
