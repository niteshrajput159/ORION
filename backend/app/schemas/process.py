from pydantic import BaseModel, Field
from typing import Optional, List
from datetime import datetime

class ProcessItem(BaseModel):
    process_id: int
    pid: int
    process_name: str
    ppid: int
    owner: str
    start_time: Optional[datetime] = None
    status: str # RUNNING, SLEEPING, STOPPED, ZOMBIE, TERMINATED, CRASHED
    is_registered: bool
    cpu_usage: float = 0.0
    memory_usage: float = 0.0
    thread_count: int = 1
    priority: int = 0
    state: str = "R"

class ProcessDetail(BaseModel):
    process_id: int
    pid: int
    process_name: str
    ppid: int
    owner: str
    command_line: Optional[str] = None
    start_time: Optional[datetime] = None
    status: str
    is_registered: bool
    cpu_usage: float
    memory_usage: float
    virtual_memory: float
    thread_count: int
    priority: int
    state: str
    context_switches: int
    page_faults: int
    open_file_descriptors: int

class ProcessHistoryItem(BaseModel):
    timestamp: datetime
    cpu_usage: float
    memory_usage: float
    thread_count: int
    priority: int
    context_switches: int
    page_faults: int
    disk_read: int
    disk_write: int
    open_file_descriptors: int

class ProcessControlRequest(BaseModel):
    action: str = Field(..., description="Action: restart_process, terminate_process, change_priority, suspend_process, resume_process")
    nice_value: Optional[int] = Field(default=10, description="Nice value for change_priority (-20 to 19)")
    mode: Optional[str] = Field(default="SIGTERM", description="Termination mode: SIGTERM or SIGKILL")

class ProcessControlResponse(BaseModel):
    success: bool
    pid: int
    action: str
    message: str
    details: Optional[dict] = None
