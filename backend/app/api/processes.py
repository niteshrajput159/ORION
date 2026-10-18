from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session
from typing import List, Optional

from app.database import get_db
from app.schemas.process import (
    ProcessItem, ProcessDetail, ProcessHistoryItem,
    ProcessControlRequest, ProcessControlResponse
)
from app.services.process_service import process_service

router = APIRouter(prefix="/processes", tags=["Processes"])

@router.get("", response_model=List[ProcessItem])
def list_processes(
    search: Optional[str] = Query(None, description="Search by process name"),
    status: Optional[str] = Query(None, description="Filter by status: RUNNING, SLEEPING, STOPPED"),
    limit: int = Query(100, ge=1, le=500),
    offset: int = Query(0, ge=0),
    db: Session = Depends(get_db)
):
    return process_service.list_processes(db, search=search, status=status, limit=limit, offset=offset)

@router.get("/{pid}", response_model=ProcessDetail)
def get_process_detail(pid: int, db: Session = Depends(get_db)):
    detail = process_service.get_process_detail(db, pid)
    if not detail:
        raise HTTPException(status_code=404, detail=f"Process with PID {pid} not found")
    return detail

@router.get("/{pid}/history", response_model=List[ProcessHistoryItem])
def get_process_history(
    pid: int,
    limit: int = Query(60, ge=10, le=300),
    db: Session = Depends(get_db)
):
    return process_service.get_process_history(db, pid, limit=limit)

@router.post("/{pid}/control", response_model=ProcessControlResponse)
def execute_process_control(
    pid: int,
    request: ProcessControlRequest,
    db: Session = Depends(get_db)
):
    return process_service.execute_controlled_action(db, pid, request)
