from sqlalchemy.orm import Session
from sqlalchemy import desc, func
from typing import List, Optional

from app.models import Recovery, Incident
from app.schemas.recovery import RecoveryItem, RecoveryChartMetrics
from app.services.db_sync_service import db_sync_service

class RecoveryService:
    @staticmethod
    def list_recoveries(
        db: Session,
        limit: int = 100,
        offset: int = 0
    ) -> List[RecoveryItem]:
        db_sync_service.sync_from_sql_file(db)
        recs = db.query(Recovery).order_by(desc(Recovery.started_at)).offset(offset).limit(limit).all()
        return [
            RecoveryItem(
                recovery_id=r.recovery_id,
                incident_id=r.incident_id,
                attempt_number=r.attempt_number,
                action=r.action,
                started_at=r.started_at,
                completed_at=r.completed_at,
                result=r.result,
                success=r.success,
                verification_details=r.verification_details,
                duration_ms=round(r.duration_ms, 2)
            )
            for r in recs
        ]

    @staticmethod
    def get_recovery_detail(db: Session, recovery_id: int) -> Optional[RecoveryItem]:
        r = db.query(Recovery).filter(Recovery.recovery_id == recovery_id).first()
        if not r:
            return None
        return RecoveryItem(
            recovery_id=r.recovery_id,
            incident_id=r.incident_id,
            attempt_number=r.attempt_number,
            action=r.action,
            started_at=r.started_at,
            completed_at=r.completed_at,
            result=r.result,
            success=r.success,
            verification_details=r.verification_details,
            duration_ms=round(r.duration_ms, 2)
        )

    @staticmethod
    def get_recovery_chart_metrics(db: Session) -> RecoveryChartMetrics:
        total = db.query(Recovery).count()
        successes = db.query(Recovery).filter(Recovery.success == True).count()
        failures = db.query(Recovery).filter(Recovery.success == False).count()
        avg_dur = db.query(func.avg(Recovery.duration_ms)).scalar() or 0.0

        incidents_with_recovery = db.query(func.count(func.distinct(Recovery.incident_id))).scalar() or 1
        attempts_per_inc = round(total / max(1, incidents_with_recovery), 2)

        return RecoveryChartMetrics(
            successful_count=successes,
            failed_count=failures,
            avg_duration_ms=round(float(avg_dur), 2),
            attempts_per_incident=attempts_per_inc
        )

recovery_service = RecoveryService()
