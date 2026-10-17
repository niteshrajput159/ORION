from sqlalchemy import Column, BigInteger, Integer, Float, String, Text, DateTime, ForeignKey, func
from sqlalchemy.orm import relationship
from app.database import Base

class ResourceEvent(Base):
    __tablename__ = "resource_event"

    event_id = Column(BigInteger, primary_key=True, index=True, autoincrement=True)
    process_id = Column(Integer, ForeignKey("process.process_id", ondelete="CASCADE"), nullable=True, index=True)
    event_type = Column(String(64), nullable=False) # THRESHOLD_EXCEEDED, SPIKE_DETECTED, LEAK_DETECTED
    resource_type = Column(String(64), nullable=False, index=True) # CPU, MEMORY, SWAP, DISK_IO, THREADS, FILE_DESCRIPTORS
    value = Column(Float, nullable=False)
    threshold = Column(Float, nullable=False)
    timestamp = Column(DateTime(timezone=True), default=func.now(), index=True)
    severity = Column(String(32), default="WARNING")
    message = Column(Text, nullable=True)

    process = relationship("Process", back_populates="resource_events")
