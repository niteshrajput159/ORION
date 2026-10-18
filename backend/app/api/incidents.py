from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session
from typing import List, Optional, Dict, Any

from app.database import get_db
from app.schemas.incident import IncidentItem, IncidentDetail
from app.services.incident_service import incident_service
from app.services.ai_service import ai_service

router = APIRouter(prefix="/incidents", tags=["Incidents"])

@router.get("", response_model=List[IncidentItem])
def list_incidents(
    incident_type: Optional[str] = Query(None, description="Filter by type: HIGH_CPU, HIGH_MEMORY, PROCESS_CRASH, etc."),
    severity: Optional[str] = Query(None, description="Filter by severity: LOW, MEDIUM, HIGH, CRITICAL"),
    status: Optional[str] = Query(None, description="Filter by status: OPEN, INVESTIGATING, RECOVERING, RESOLVED, ESCALATED"),
    limit: int = Query(100, ge=1, le=500),
    offset: int = Query(0, ge=0),
    db: Session = Depends(get_db)
):
    return incident_service.list_incidents(
        db,
        incident_type=incident_type,
        severity=severity,
        status=status,
        limit=limit,
        offset=offset
    )

@router.get("/{incident_id}", response_model=IncidentDetail)
def get_incident_detail(incident_id: int, db: Session = Depends(get_db)):
    detail = incident_service.get_incident_detail(db, incident_id)
    if not detail:
        raise HTTPException(status_code=404, detail=f"Incident #{incident_id} not found")
    return detail

@router.get("/{incident_id}/explain")
def get_ai_incident_explanation(incident_id: int, db: Session = Depends(get_db)) -> Dict[str, Any]:
    explanation = ai_service.explain_incident(db, incident_id)
    if "error" in explanation:
        raise HTTPException(status_code=404, detail=explanation["error"])
    return explanation

@router.post("/{incident_id}/resolve")
def resolve_incident(incident_id: int, db: Session = Depends(get_db)) -> Dict[str, Any]:
    success = incident_service.resolve_incident(db, incident_id)
    if not success:
        raise HTTPException(status_code=404, detail=f"Incident #{incident_id} not found")
    return {"success": True, "incident_id": incident_id, "status": "RESOLVED", "message": f"Incident #{incident_id} successfully marked as RESOLVED."}

@router.post("/{incident_id}/recover")
def recover_incident(incident_id: int, db: Session = Depends(get_db)) -> Dict[str, Any]:
    res = incident_service.recover_incident(db, incident_id)
    if not res.get("success"):
        raise HTTPException(status_code=404, detail=res.get("error", "Recovery execution failed"))
    return res

@router.post("/resolve-all")
def resolve_all_incidents(db: Session = Depends(get_db)) -> Dict[str, Any]:
    count = incident_service.resolve_all_incidents(db)
    return {"success": True, "resolved_count": count, "message": f"Successfully resolved {count} incidents."}

