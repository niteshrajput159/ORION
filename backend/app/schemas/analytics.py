from pydantic import BaseModel
from typing import List, Dict

class IncidentDistributionItem(BaseModel):
    incident_type: str
    count: int

class SeverityDistributionItem(BaseModel):
    severity: str
    count: int

class RecoveryOutcomeItem(BaseModel):
    action: str
    success_count: int
    failure_count: int
    avg_duration_ms: float

class AnalyticsSummaryResponse(BaseModel):
    total_incidents: int
    active_incidents: int
    resolved_incidents: int
    escalated_incidents: int
    total_recoveries: int
    recovery_success_rate: float
    avg_recovery_duration_ms: float
    incident_types: List[IncidentDistributionItem]
    severities: List[SeverityDistributionItem]
    recovery_outcomes: List[RecoveryOutcomeItem]
