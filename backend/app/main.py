import os
import logging
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware

from app.database import engine, Base, SessionLocal
from app.models import Policy
from app.api import (
    system_router,
    processes_router,
    incidents_router,
    recovery_router,
    resources_router,
    events_router,
    policies_router,
    deadlocks_router,
    analytics_router,
    workloads_router
)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] [%(levelname)s] [%(name)s] %(message)s"
)
logger = logging.getLogger("orion.main")

@asynccontextmanager
async def lifespan(app: FastAPI):
    logger.info("Initializing Orion Backend API service...")
    # Create tables if not existing
    try:
        Base.metadata.create_all(bind=engine)
        logger.info("Database tables verified.")

        # Seed initial policies if empty
        db = SessionLocal()
        try:
            if db.query(Policy).count() == 0:
                logger.info("Seeding default recovery policies...")
                default_policies = [
                    Policy(incident_type="HIGH_CPU", severity="MEDIUM", action="LOWER_PRIORITY", priority=10, max_attempts=2, timeout_seconds=5, cpu_threshold=85.0, persistence_count=4, enabled=True),
                    Policy(incident_type="HIGH_CPU", severity="CRITICAL", action="SUSPEND", priority=20, max_attempts=1, timeout_seconds=5, cpu_threshold=95.0, persistence_count=4, enabled=True),
                    Policy(incident_type="HIGH_MEMORY", severity="HIGH", action="TERMINATE_SIGTERM", priority=10, max_attempts=2, timeout_seconds=5, memory_threshold=80.0, persistence_count=4, enabled=True),
                    Policy(incident_type="HIGH_MEMORY", severity="CRITICAL", action="TERMINATE_SIGKILL", priority=20, max_attempts=1, timeout_seconds=3, memory_threshold=90.0, persistence_count=2, enabled=True),
                    Policy(incident_type="PROCESS_CRASH", severity="HIGH", action="RESTART", priority=10, max_attempts=3, timeout_seconds=5, persistence_count=1, enabled=True),
                    Policy(incident_type="CRASH_LOOP", severity="CRITICAL", action="ALERT_ONLY", priority=30, max_attempts=0, timeout_seconds=0, persistence_count=1, enabled=True),
                    Policy(incident_type="HUNG_PROCESS", severity="HIGH", action="TERMINATE_SIGTERM", priority=15, max_attempts=2, timeout_seconds=5, persistence_count=3, enabled=True),
                    Policy(incident_type="DEADLOCK", severity="CRITICAL", action="RESOLVE_DEADLOCK", priority=30, max_attempts=2, timeout_seconds=5, persistence_count=1, enabled=True),
                    Policy(incident_type="EXCESSIVE_THREADS", severity="HIGH", action="TERMINATE_SIGTERM", priority=10, max_attempts=1, timeout_seconds=5, persistence_count=3, enabled=True),
                    Policy(incident_type="EXCESSIVE_FILE_DESCRIPTORS", severity="HIGH", action="TERMINATE_SIGTERM", priority=10, max_attempts=1, timeout_seconds=5, persistence_count=3, enabled=True),
                ]
                db.add_all(default_policies)
                db.commit()
                logger.info("Default policies seeded.")
        finally:
            db.close()
    except Exception as e:
        logger.warning("Database initialization notice: %s", e)

    yield
    logger.info("Shutting down Orion Backend API service.")

app = FastAPI(
    title="Orion API",
    description="Intelligent OS Behavior & Recovery Platform REST API",
    version="1.0.0",
    lifespan=lifespan
)

# Enable CORS for React Frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Register API Routers under /api
api_prefix = "/api"
app.include_router(system_router, prefix=api_prefix)
app.include_router(processes_router, prefix=api_prefix)
app.include_router(incidents_router, prefix=api_prefix)
app.include_router(recovery_router, prefix=api_prefix)
app.include_router(resources_router, prefix=api_prefix)
app.include_router(events_router, prefix=api_prefix)
app.include_router(policies_router, prefix=api_prefix)
app.include_router(deadlocks_router, prefix=api_prefix)
app.include_router(analytics_router, prefix=api_prefix)
app.include_router(workloads_router, prefix=api_prefix)

@app.get("/", response_class=HTMLResponse)
def root():
    template_path = os.path.join(os.path.dirname(__file__), "templates", "dashboard.html")
    if os.path.exists(template_path):
        with open(template_path, "r", encoding="utf-8") as f:
            return HTMLResponse(content=f.read())
    return HTMLResponse("<h1>Orion Backend API Running</h1><p><a href='/docs'>Swagger API Docs</a></p>")

@app.get("/api/info")
def api_info():
    return {
        "service": "Orion Intelligent OS Behavior & Recovery API",
        "version": "1.0.0",
        "status": "RUNNING",
        "documentation": "/docs"
    }

@app.get("/healthz")
def healthz():
    return {"status": "ok"}
