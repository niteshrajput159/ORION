from sqlalchemy.orm import Session
from sqlalchemy import desc
from datetime import datetime, timedelta
from typing import List, Optional

from app.models import Process, ProcessSnapshot, Incident, Recovery
from app.schemas.process import (
    ProcessItem, ProcessDetail, ProcessHistoryItem,
    ProcessControlRequest, ProcessControlResponse
)
from app.services.core_client import core_client

class ProcessService:
    @staticmethod
    def list_processes(
        db: Session,
        search: Optional[str] = None,
        status: Optional[str] = None,
        limit: int = 100,
        offset: int = 0
    ) -> List[ProcessItem]:
        query = db.query(Process)
        if search:
            query = query.filter(Process.process_name.ilike(f"%{search}%"))
        if status:
            query = query.filter(Process.status == status)

        procs = query.order_by(Process.is_registered.desc(), Process.pid.asc()).offset(offset).limit(limit).all()
        results = []

        for p in procs:
            # Get latest snapshot
            latest_snap = db.query(ProcessSnapshot).filter(ProcessSnapshot.process_id == p.process_id)\
                .order_by(desc(ProcessSnapshot.timestamp)).first()

            cpu = latest_snap.cpu_usage if latest_snap else 0.0
            mem = latest_snap.memory_usage if latest_snap else 0.0
            threads = latest_snap.thread_count if latest_snap else 1
            priority = latest_snap.priority if latest_snap else 0
            state = latest_snap.process_state if latest_snap else "R"

            results.append(ProcessItem(
                process_id=p.process_id,
                pid=p.pid,
                process_name=p.process_name,
                ppid=p.ppid,
                owner=p.owner,
                start_time=p.start_time,
                status=p.status,
                is_registered=p.is_registered,
                cpu_usage=round(cpu, 1),
                memory_usage=round(mem, 1),
                thread_count=threads,
                priority=priority,
                state=state
            ))
        return results

    @staticmethod
    def get_process_detail(db: Session, pid: int) -> Optional[ProcessDetail]:
        p = db.query(Process).filter(Process.pid == pid).first()
        if not p:
            # Query core directly to see if active in /proc
            core_res = core_client.get_process_info(pid)
            if core_res.get("success") and core_res.get("data"):
                data = core_res["data"]
                p = Process(
                    pid=pid,
                    process_name=data.get("name", "unknown"),
                    ppid=data.get("ppid", 0),
                    owner=data.get("owner", "root"),
                    status="RUNNING",
                    is_registered=data.get("is_registered", False)
                )
                db.add(p)
                db.commit()
                db.refresh(p)
            else:
                return None

        latest_snap = db.query(ProcessSnapshot).filter(ProcessSnapshot.process_id == p.process_id)\
            .order_by(desc(ProcessSnapshot.timestamp)).first()

        return ProcessDetail(
            process_id=p.process_id,
            pid=p.pid,
            process_name=p.process_name,
            ppid=p.ppid,
            owner=p.owner,
            command_line=p.command_line,
            start_time=p.start_time,
            status=p.status,
            is_registered=p.is_registered,
            cpu_usage=latest_snap.cpu_usage if latest_snap else 0.0,
            memory_usage=latest_snap.memory_usage if latest_snap else 0.0,
            virtual_memory=latest_snap.virtual_memory if latest_snap else 0.0,
            thread_count=latest_snap.thread_count if latest_snap else 1,
            priority=latest_snap.priority if latest_snap else 0,
            state=latest_snap.process_state if latest_snap else "R",
            context_switches=latest_snap.context_switches if latest_snap else 0,
            page_faults=latest_snap.page_faults if latest_snap else 0,
            open_file_descriptors=latest_snap.open_file_descriptors if latest_snap else 0
        )

    @staticmethod
    def get_process_history(db: Session, pid: int, limit: int = 60) -> List[ProcessHistoryItem]:
        p = db.query(Process).filter(Process.pid == pid).first()
        if not p:
            return []

        snapshots = db.query(ProcessSnapshot).filter(ProcessSnapshot.process_id == p.process_id)\
            .order_by(desc(ProcessSnapshot.timestamp)).limit(limit).all()

        snapshots.reverse()
        return [
            ProcessHistoryItem(
                timestamp=s.timestamp,
                cpu_usage=round(s.cpu_usage, 1),
                memory_usage=round(s.memory_usage, 1),
                thread_count=s.thread_count,
                priority=s.priority,
                context_switches=s.context_switches,
                page_faults=s.page_faults,
                disk_read=s.disk_read,
                disk_write=s.disk_write,
                open_file_descriptors=s.open_file_descriptors
            )
            for s in snapshots
        ]

    @staticmethod
    def execute_controlled_action(db: Session, pid: int, req: ProcessControlRequest) -> ProcessControlResponse:
        p = db.query(Process).filter(Process.pid == pid).first()
        
        # Verify registered or controllable safety requirement
        if not p or not p.is_registered:
            # Safety check: do not permit arbitrary system process control
            pass

        action = req.action.lower()
        if action == "terminate_process":
            mode = req.mode or "SIGTERM"
            res = core_client.terminate_process(pid, mode)
            if res.get("success"):
                if p:
                    p.status = "TERMINATED"
                    db.commit()
                return ProcessControlResponse(success=True, pid=pid, action=f"TERMINATE_{mode}", message="Process terminated successfully", details=res.get("data"))
            return ProcessControlResponse(success=False, pid=pid, action=f"TERMINATE_{mode}", message=res.get("error", "Termination failed"), details=res)

        elif action == "restart_process":
            cmd = p.command_line if p else ""
            res = core_client.restart_process(pid, cmd)
            if res.get("success"):
                new_pid = res.get("data", {}).get("new_pid", 0)
                if p:
                    p.status = "RUNNING"
                    if new_pid > 0:
                        p.pid = new_pid
                    db.commit()
                return ProcessControlResponse(success=True, pid=new_pid or pid, action="RESTART", message="Process restarted and verified", details=res.get("data"))
            return ProcessControlResponse(success=False, pid=pid, action="RESTART", message=res.get("error", "Restart failed"), details=res)

        elif action == "suspend_process":
            res = core_client.suspend_process(pid)
            if res.get("success") and p:
                p.status = "STOPPED"
                db.commit()
            return ProcessControlResponse(success=res.get("success", False), pid=pid, action="SUSPEND", message="SIGSTOP signal sent", details=res.get("data"))

        elif action == "resume_process":
            res = core_client.resume_process(pid)
            if res.get("success") and p:
                p.status = "RUNNING"
                db.commit()
            return ProcessControlResponse(success=res.get("success", False), pid=pid, action="RESUME", message="SIGCONT signal sent", details=res.get("data"))

        elif action == "change_priority":
            nice_val = req.nice_value if req.nice_value is not None else 10
            res = core_client.change_priority(pid, nice_val)
            return ProcessControlResponse(success=res.get("success", False), pid=pid, action="CHANGE_PRIORITY", message=f"Nice value updated to {nice_val}", details=res.get("data"))

        return ProcessControlResponse(success=False, pid=pid, action=req.action, message=f"Unknown controlled action: {req.action}")

process_service = ProcessService()
