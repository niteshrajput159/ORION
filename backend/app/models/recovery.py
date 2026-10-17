from sqlalchemy import Column, Integer, String, Text, Boolean, Float, DateTime, ForeignKey, func
from sqlalchemy.orm import relationship
from app.database import Base

class Recovery(Base):
    __tablename__ = "recovery"

    recovery_id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    incident_id = Column(Integer, ForeignKey("incident.incident_id", ondelete="CASCADE"), nullable=False, index=True)
    attempt_number = Column(Integer, default=1, nullable=False)
    action = Column(String(64), nullable=False)
    started_at = Column(DateTime(timezone=True), default=func.now(), index=True)
    completed_at = Column(DateTime(timezone=True), nullable=True)
    result = Column(String(64), nullable=False) # VERIFIED_SUCCESS, VERIFIED_FAILURE, TIMED_OUT, ESCALATED_SIGKILL
    success = Column(Boolean, default=False, nullable=False, index=True)
    verification_details = Column(Text, nullable=True)
    duration_ms = Column(Float, default=0.0)

    incident = relationship("Incident", back_populates="recoveries")
