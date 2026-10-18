from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from typing import List

from app.database import get_db
from app.schemas.system import SystemHealthResponse, SystemOverviewChart
from app.services.system_service import system_service

router = APIRouter(prefix="/system", tags=["System"])

@router.get("/health", response_model=SystemHealthResponse)
def get_system_health(db: Session = Depends(get_db)):
    return system_service.get_system_health(db)

@router.get("/overview", response_model=List[SystemOverviewChart])
def get_system_overview_timeline(db: Session = Depends(get_db)):
    return system_service.get_overview_timeline(db)
