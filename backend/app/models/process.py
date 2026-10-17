from sqlalchemy import Column, Integer, String, Boolean, DateTime, Text, func
from sqlalchemy.orm import relationship
from app.database import Base

class Process(Base):
    __tablename__ = "process"

    process_id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    pid = Column(Integer, nullable=False, index=True)
    process_name = Column(String(255), nullable=False, index=True)
    ppid = Column(Integer, default=0)
    owner = Column(String(64), default="root")
    start_time = Column(DateTime(timezone=True), default=func.now())
    status = Column(String(32), default="RUNNING", index=True)
    is_registered = Column(Boolean, default=True)
    command_line = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), default=func.now())

    snapshots = relationship("ProcessSnapshot", back_populates="process", cascade="all, delete-orphan")
    incidents = relationship("Incident", back_populates="process")
    resource_events = relationship("ResourceEvent", back_populates="process", cascade="all, delete-orphan")
