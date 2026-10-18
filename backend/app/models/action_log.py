from sqlalchemy import Column, Integer, String, Text, DateTime, ForeignKey, func, JSON
from sqlalchemy.orm import relationship
from app.database import Base

class ActionLog(Base):
    __tablename__ = "action_log"

    action_id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    incident_id = Column(Integer, ForeignKey("incident.incident_id", ondelete="CASCADE"), nullable=False, index=True)
    policy_id = Column(Integer, ForeignKey("policy.policy_id", ondelete="SET NULL"), nullable=True, index=True)
    action = Column(String(64), nullable=False)
    timestamp = Column(DateTime(timezone=True), default=func.now(), index=True)
    result = Column(String(32), nullable=False) # SUCCESS, FAILED, TIMED_OUT, ESCALATED
    error_message = Column(Text, nullable=True)
    details = Column(JSON, default=dict)

    incident = relationship("Incident", back_populates="action_logs")
    policy = relationship("Policy", back_populates="action_logs")
