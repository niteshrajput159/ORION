from sqlalchemy import Column, Integer, String, Boolean, Float, DateTime, func
from sqlalchemy.orm import relationship
from app.database import Base

class Policy(Base):
    __tablename__ = "policy"

    policy_id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    incident_type = Column(String(64), nullable=False, index=True)
    severity = Column(String(32), nullable=False, default="MEDIUM")
    action = Column(String(64), nullable=False)
    priority = Column(Integer, default=10)
    max_attempts = Column(Integer, default=3)
    timeout_seconds = Column(Integer, default=10)
    cpu_threshold = Column(Float, default=90.0)
    memory_threshold = Column(Float, default=85.0)
    persistence_count = Column(Integer, default=4)
    enabled = Column(Boolean, default=True, index=True)
    created_at = Column(DateTime(timezone=True), default=func.now())
    updated_at = Column(DateTime(timezone=True), default=func.now(), onupdate=func.now())

    action_logs = relationship("ActionLog", back_populates="policy")
