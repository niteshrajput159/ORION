from sqlalchemy import Column, BigInteger, Integer, Float, String, DateTime, ForeignKey, func
from sqlalchemy.orm import relationship
from app.database import Base

class ProcessSnapshot(Base):
    __tablename__ = "process_snapshot"

    snapshot_id = Column(BigInteger, primary_key=True, index=True, autoincrement=True)
    process_id = Column(Integer, ForeignKey("process.process_id", ondelete="CASCADE"), nullable=False, index=True)
    timestamp = Column(DateTime(timezone=True), default=func.now(), index=True)
    cpu_usage = Column(Float, nullable=False, default=0.0)
    memory_usage = Column(Float, nullable=False, default=0.0) # RSS MB
    virtual_memory = Column(Float, nullable=False, default=0.0) # VmSize MB
    process_state = Column(String(16), default="R")
    thread_count = Column(Integer, default=1)
    priority = Column(Integer, default=0)
    context_switches = Column(BigInteger, default=0)
    page_faults = Column(BigInteger, default=0)
    disk_read = Column(BigInteger, default=0)
    disk_write = Column(BigInteger, default=0)
    open_file_descriptors = Column(Integer, default=0)

    process = relationship("Process", back_populates="snapshots")
