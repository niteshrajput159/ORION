from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session
from typing import List, Optional

from app.database import get_db
from app.services.workload_service import workload_service

router = APIRouter(prefix="/workloads", tags=["Workloads"])

@router.get("")
def list_workloads():
    return workload_service.get_workload_status()

@router.post("/{name}/start")
def start_workload(
    name: str,
    args: Optional[List[str]] = Query(None),
    db: Session = Depends(get_db)
):
    res = workload_service.start_workload(db, name, args)
    if not res.get("success"):
        raise HTTPException(status_code=400, detail=res.get("error", "Failed to start workload"))
    return res

@router.post("/{name}/stop")
def stop_workload(
    name: str,
    db: Session = Depends(get_db)
):
    res = workload_service.stop_workload(db, name)
    return res

@router.post("/{name}/recover")
def remediate_workload(
    name: str,
    db: Session = Depends(get_db)
):
    res = workload_service.remediate_workload(db, name)
    return res

