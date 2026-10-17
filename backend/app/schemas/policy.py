from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime

class PolicyItem(BaseModel):
    policy_id: int
    incident_type: str
    severity: str
    action: str
    priority: int
    max_attempts: int
    timeout_seconds: int
    cpu_threshold: float
    memory_threshold: float
    persistence_count: int
    enabled: bool
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None

class PolicyUpdate(BaseModel):
    cpu_threshold: Optional[float] = Field(None, ge=10.0, le=100.0)
    memory_threshold: Optional[float] = Field(None, ge=10.0, le=100.0)
    persistence_count: Optional[int] = Field(None, ge=1, le=20)
    max_attempts: Optional[int] = Field(None, ge=0, le=10)
    timeout_seconds: Optional[int] = Field(None, ge=1, le=60)
    action: Optional[str] = Field(None)
    enabled: Optional[bool] = None
