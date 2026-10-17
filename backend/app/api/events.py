from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session
from sqlalchemy import desc
from typing import List, Optional

from app.database import get_db
from app.models import ResourceEvent, Process
from app.schemas.resource import ResourceEventItem

router = APIRouter(prefix="/events", tags=["Events"])

@router.get("", response_model=List[ResourceEventItem])
def list_events(
    resource_type: Optional[str] = Query(None, description="Filter: CPU, MEMORY, SWAP, THREADS, FILE_DESCRIPTORS"),
    severity: Optional[str] = Query(None, description="Filter: WARNING, CRITICAL"),
    limit: int = Query(100, ge=1, le=500),
    offset: int = Query(0, ge=0),
    db: Session = Depends(get_db)
):
    query = db.query(ResourceEvent)
    if resource_type:
        query = query.filter(ResourceEvent.resource_type == resource_type)
    if severity:
        query = query.filter(ResourceEvent.severity == severity)

    events = query.order_by(desc(ResourceEvent.timestamp)).offset(offset).limit(limit).all()

    return [
        ResourceEventItem(
            event_id=ev.event_id,
            process_id=ev.process_id,
            pid=ev.process.pid if ev.process else None,
            process_name=ev.process.process_name if ev.process else "system",
            event_type=ev.event_type,
            resource_type=ev.resource_type,
            value=round(ev.value, 1),
            threshold=round(ev.threshold, 1),
            timestamp=ev.timestamp,
            severity=ev.severity,
            message=ev.message
        )
        for ev in events
    ]
