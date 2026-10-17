from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from typing import List

from app.database import get_db
from app.schemas.policy import PolicyItem, PolicyUpdate
from app.services.policy_service import policy_service

router = APIRouter(prefix="/policies", tags=["Policies"])

@router.get("", response_model=List[PolicyItem])
def list_policies(db: Session = Depends(get_db)):
    return policy_service.list_policies(db)

@router.patch("/{policy_id}", response_model=PolicyItem)
def update_policy(
    policy_id: int,
    update_data: PolicyUpdate,
    db: Session = Depends(get_db)
):
    updated = policy_service.update_policy(db, policy_id, update_data)
    if not updated:
        raise HTTPException(status_code=404, detail=f"Policy #{policy_id} not found")
    return updated
