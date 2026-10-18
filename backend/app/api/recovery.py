from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session
from typing import List

from app.database import get_db
from app.schemas.recovery import RecoveryItem, RecoveryChartMetrics
from app.services.recovery_service import recovery_service

router = APIRouter(prefix="/recovery", tags=["Recovery"])

@router.get("", response_model=List[RecoveryItem])
def list_recoveries(
    limit: int = Query(100, ge=1, le=500),
    offset: int = Query(0, ge=0),
    db: Session = Depends(get_db)
):
    return recovery_service.list_recoveries(db, limit=limit, offset=offset)

@router.get("/metrics", response_model=RecoveryChartMetrics)
def get_recovery_metrics(db: Session = Depends(get_db)):
    return recovery_service.get_recovery_chart_metrics(db)

@router.get("/{recovery_id}", response_model=RecoveryItem)
def get_recovery_detail(recovery_id: int, db: Session = Depends(get_db)):
    rec = recovery_service.get_recovery_detail(db, recovery_id)
    if not rec:
        raise HTTPException(status_code=404, detail=f"Recovery #{recovery_id} not found")
    return rec
