from pydantic import BaseModel
from typing import List, Optional

class DeadlockGraphEdge(BaseModel):
    waiting_thread: int
    holding_thread: int
    resource: str

class DeadlockReport(BaseModel):
    has_deadlock: bool
    threads: List[int]
    edges: List[DeadlockGraphEdge]
    cycle: List[int]
    victim_thread: int
    strategy: str
    action: str
    verification: str
    final_result: str
