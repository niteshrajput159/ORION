from pydantic import BaseModel
from typing import Optional
from datetime import datetime

class ResourceEventItem(BaseModel):
    event_id: int
    process_id: Optional[int] = None
    pid: Optional[int] = None
    process_name: Optional[str] = None
    event_type: str
    resource_type: str
    value: float
    threshold: float
    timestamp: datetime
    severity: str
    message: Optional[str] = None

class ResourceHistoricalPoint(BaseModel):
    timestamp: str
    cpu_percent: float
    memory_percent: float
    threads: int
    open_fds: int
    process_count: int
