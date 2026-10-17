from sqlalchemy import Column, Integer, String, Text, DateTime, ForeignKey, func, JSON
from sqlalchemy.orm import relationship
from app.database import Base

class Incident(Base):
    __tablename__ = "incident"

    incident_id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    process_id = Column(Integer, ForeignKey("process.process_id", ondelete="SET NULL"), nullable=True, index=True)
    incident_type = Column(String(64), nullable=False, index=True)
    severity = Column(String(32), nullable=False, default="MEDIUM", index=True)
    detected_at = Column(DateTime(timezone=True), default=func.now(), index=True)
    status = Column(String(32), nullable=False, default="OPEN", index=True) # OPEN, INVESTIGATING, RECOVERING, RESOLVED, ESCALATED, FAILED
    description = Column(Text, nullable=True)
    resolved_at = Column(DateTime(timezone=True), nullable=True)
    incident_metadata = Column("metadata", JSON, default=dict)

    process = relationship("Process", back_populates="incidents")
    action_logs = relationship("ActionLog", back_populates="incident", cascade="all, delete-orphan")
    recoveries = relationship("Recovery", back_populates="incident", cascade="all, delete-orphan")
