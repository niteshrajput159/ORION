from pydantic import BaseModel
from typing import Optional

class SystemHealthResponse(BaseModel):
    health: str # HEALTHY, WARNING, CRITICAL
    cpu_usage_percent: float
    memory_usage_percent: float
    memory_total_mb: float
    memory_used_mb: float
    memory_free_mb: float
    swap_used_mb: float
    active_processes: int
    active_incidents: int
    recovery_success_rate: float
    failed_recoveries: int

class SystemOverviewChart(BaseModel):
    timestamp: str
    cpu_percent: float
    memory_percent: float
