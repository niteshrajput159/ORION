from sqlalchemy.orm import Session
from typing import List, Optional

from app.models import Policy
from app.schemas.policy import PolicyItem, PolicyUpdate

ALLOWED_ACTIONS = {
    "LOWER_PRIORITY",
    "SUSPEND",
    "RESUME",
    "RESTART",
    "TERMINATE_SIGTERM",
    "TERMINATE_SIGKILL",
    "RESOLVE_DEADLOCK",
    "ALERT_ONLY"
}

class PolicyService:
    @staticmethod
    def list_policies(db: Session) -> List[PolicyItem]:
        policies = db.query(Policy).order_by(Policy.policy_id.asc()).all()
        return [
            PolicyItem(
                policy_id=p.policy_id,
                incident_type=p.incident_type,
                severity=p.severity,
                action=p.action,
                priority=p.priority,
                max_attempts=p.max_attempts,
                timeout_seconds=p.timeout_seconds,
                cpu_threshold=p.cpu_threshold,
                memory_threshold=p.memory_threshold,
                persistence_count=p.persistence_count,
                enabled=p.enabled,
                created_at=p.created_at,
                updated_at=p.updated_at
            )
            for p in policies
        ]

    @staticmethod
    def update_policy(db: Session, policy_id: int, update_data: PolicyUpdate) -> Optional[PolicyItem]:
        p = db.query(Policy).filter(Policy.policy_id == policy_id).first()
        if not p:
            return None

        if update_data.cpu_threshold is not None:
            p.cpu_threshold = update_data.cpu_threshold
        if update_data.memory_threshold is not None:
            p.memory_threshold = update_data.memory_threshold
        if update_data.persistence_count is not None:
            p.persistence_count = update_data.persistence_count
        if update_data.max_attempts is not None:
            p.max_attempts = update_data.max_attempts
        if update_data.timeout_seconds is not None:
            p.timeout_seconds = update_data.timeout_seconds
        if update_data.enabled is not None:
            p.enabled = update_data.enabled

        # Security check: action must be in explicit allowed whitelist
        if update_data.action is not None:
            cleaned_action = update_data.action.upper().strip()
            if cleaned_action in ALLOWED_ACTIONS:
                p.action = cleaned_action

        db.commit()
        db.refresh(p)

        return PolicyItem(
            policy_id=p.policy_id,
            incident_type=p.incident_type,
            severity=p.severity,
            action=p.action,
            priority=p.priority,
            max_attempts=p.max_attempts,
            timeout_seconds=p.timeout_seconds,
            cpu_threshold=p.cpu_threshold,
            memory_threshold=p.memory_threshold,
            persistence_count=p.persistence_count,
            enabled=p.enabled,
            created_at=p.created_at,
            updated_at=p.updated_at
        )

policy_service = PolicyService()
