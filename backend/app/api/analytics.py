from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from sqlalchemy import func

from app.database import get_db
from app.models import Incident, Recovery
from app.schemas.analytics import (
    AnalyticsSummaryResponse, IncidentDistributionItem,
    SeverityDistributionItem, RecoveryOutcomeItem
)

router = APIRouter(prefix="/analytics", tags=["Analytics"])

@router.get("/summary", response_model=AnalyticsSummaryResponse)
def get_analytics_summary(db: Session = Depends(get_db)):
    total_incidents = db.query(Incident).count()
    active_incidents = db.query(Incident).filter(Incident.status.in_(["OPEN", "INVESTIGATING", "RECOVERING"])).count()
    resolved_incidents = db.query(Incident).filter(Incident.status == "RESOLVED").count()
    escalated_incidents = db.query(Incident).filter(Incident.status == "ESCALATED").count()

    total_recoveries = db.query(Recovery).count()
    successful_recoveries = db.query(Recovery).filter(Recovery.success == True).count()
    success_rate = (successful_recoveries / total_recoveries * 100.0) if total_recoveries > 0 else 0.0
    avg_dur = db.query(func.avg(Recovery.duration_ms)).scalar() or 0.0

    # Incident types distribution
    type_counts = db.query(Incident.incident_type, func.count(Incident.incident_id))\
        .group_by(Incident.incident_type).all()
    type_items = [IncidentDistributionItem(incident_type=t, count=c) for t, c in type_counts]

    # Severity distribution
    sev_counts = db.query(Incident.severity, func.count(Incident.incident_id))\
        .group_by(Incident.severity).all()
    sev_items = [SeverityDistributionItem(severity=s, count=c) for s, c in sev_counts]

    # Recovery outcomes per action
    action_counts = db.query(
        Recovery.action,
        func.count(func.nullif(Recovery.success, False)),
        func.count(func.nullif(Recovery.success, True)),
        func.avg(Recovery.duration_ms)
    ).group_by(Recovery.action).all()

    action_items = [
        RecoveryOutcomeItem(
            action=act,
            success_count=succ or 0,
            failure_count=fail or 0,
            avg_duration_ms=round(float(avg_d or 0.0), 2)
        )
        for act, succ, fail, avg_d in action_counts
    ]

    return AnalyticsSummaryResponse(
        total_incidents=total_incidents,
        active_incidents=active_incidents,
        resolved_incidents=resolved_incidents,
        escalated_incidents=escalated_incidents,
        total_recoveries=total_recoveries,
        recovery_success_rate=round(success_rate, 1),
        avg_recovery_duration_ms=round(float(avg_dur), 2),
        incident_types=type_items,
        severities=sev_items,
        recovery_outcomes=action_items
    )

@router.get("/incidents")
def get_incident_analytics(db: Session = Depends(get_db)):
    counts = db.query(Incident.incident_type, func.count(Incident.incident_id))\
        .group_by(Incident.incident_type).all()
    return [{"type": t, "count": c} for t, c in counts]

@router.get("/recovery")
def get_recovery_analytics(db: Session = Depends(get_db)):
    successes = db.query(Recovery).filter(Recovery.success == True).count()
    failures = db.query(Recovery).filter(Recovery.success == False).count()
    return {
        "successful": successes,
        "failed": failures,
        "total": successes + failures,
        "success_rate": round(successes / max(1, successes + failures) * 100.0, 1)
    }
