from pydantic import BaseModel
from typing import Optional
from datetime import datetime

class RecoveryItem(BaseModel):
    recovery_id: int
    incident_id: int
    attempt_number: int
    action: str
    started_at: datetime
    completed_at: Optional[datetime] = None
    result: str # VERIFIED_SUCCESS, VERIFIED_FAILURE, TIMED_OUT, ESCALATED_SIGKILL
    success: bool
    verification_details: Optional[str] = None
    duration_ms: float

class RecoveryChartMetrics(BaseModel):
    successful_count: int
    failed_count: int
    avg_duration_ms: float
    attempts_per_incident: float
