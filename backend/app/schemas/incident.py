from pydantic import BaseModel
from typing import Optional, List, Dict, Any
from datetime import datetime

class IncidentItem(BaseModel):
    incident_id: int
    process_id: Optional[int] = None
    pid: Optional[int] = None
    process_name: Optional[str] = None
    incident_type: str # HIGH_CPU, HIGH_MEMORY, PROCESS_CRASH, CRASH_LOOP, HUNG_PROCESS, DEADLOCK, HIGH_IO, EXCESSIVE_THREADS, EXCESSIVE_FILE_DESCRIPTORS
    severity: str # LOW, MEDIUM, HIGH, CRITICAL
    detected_at: datetime
    status: str # OPEN, INVESTIGATING, RECOVERING, RESOLVED, ESCALATED, FAILED
    description: Optional[str] = None
    recovery_status: Optional[str] = None

class TimelineStep(BaseModel):
    step_title: str
    timestamp: str
    status: str # COMPLETED, IN_PROGRESS, FAILED, PENDING
    description: str

class IncidentDetail(BaseModel):
    incident_id: int
    process_id: Optional[int] = None
    pid: Optional[int] = None
    process_name: Optional[str] = None
    incident_type: str
    severity: str
    detected_at: datetime
    resolved_at: Optional[datetime] = None
    status: str
    description: Optional[str] = None
    metadata: Dict[str, Any] = {}
    timeline: List[TimelineStep] = []
    policy_name: Optional[str] = None
    action_taken: Optional[str] = None
    attempts: int = 0
    verification_result: Optional[str] = None
    final_result: Optional[str] = None
