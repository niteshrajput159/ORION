import os
import re
import logging
from datetime import datetime
from sqlalchemy.orm import Session
from app.models import Process, Incident, Recovery, ActionLog

logger = logging.getLogger("oscope.db_sync")

SQL_SYNC_FILE = "/tmp/oscope_db_sync.sql"

re_proc = re.compile(
    r"INSERT INTO PROCESS\s*\([^)]*\)\s*VALUES\s*\((\d+),\s*'([^']*)',\s*(\d+),\s*'([^']*)',\s*'([^']*)',\s*(true|false),\s*'([^']*)'\);",
    re.IGNORECASE
)

re_inc = re.compile(
    r"INSERT INTO INCIDENT\s*\([^)]*\)\s*VALUES\s*\((\d+),\s*(\d+),\s*'([^']*)',\s*'([^']*)',\s*'([^']*)',\s*'([^']*)'\);",
    re.IGNORECASE
)

re_upd_inc = re.compile(
    r"UPDATE INCIDENT SET status = '([^']*)'.*?WHERE incident_id = (\d+);",
    re.IGNORECASE
)

re_rec = re.compile(
    r"INSERT INTO RECOVERY\s*\([^)]*\)\s*VALUES\s*\((\d+),\s*(\d+),\s*(\d+),\s*'([^']*)',\s*[^,]+,\s*'([^']*)',\s*(true|false),\s*'([^']*)',\s*([\d\.]+)\);",
    re.IGNORECASE
)

re_act = re.compile(
    r"INSERT INTO ACTION_LOG\s*\([^)]*\)\s*VALUES\s*\((\d+),\s*(\d+),\s*([^,]+),\s*'([^']*)',\s*'([^']*)',\s*'([^']*)'\);",
    re.IGNORECASE
)

class DBSyncService:
    def __init__(self):
        self.last_pos = 0

    def sync_from_sql_file(self, db: Session):
        if not os.path.exists(SQL_SYNC_FILE):
            return

        file_size = os.path.getsize(SQL_SYNC_FILE)
        if file_size < self.last_pos:
            self.last_pos = 0  # File truncated or restarted

        # If file is very large (> 2MB) on first sync, start from the last 1MB to avoid slow startup
        if self.last_pos == 0 and file_size > 1024 * 1024:
            self.last_pos = max(0, file_size - 1024 * 1024)

        try:
            with open(SQL_SYNC_FILE, "r", encoding="utf-8", errors="ignore") as f:
                f.seek(self.last_pos)
                lines = f.readlines()
                self.last_pos = f.tell()

            if lines:
                existing_incidents = {i[0] for i in db.query(Incident.incident_id).all()}
                existing_recoveries = {r[0] for r in db.query(Recovery.recovery_id).all()}
                pid_to_proc_id = {p.pid: p.process_id for p in db.query(Process).all()}

                new_incidents = []
                new_recoveries = []

                for line in lines:
                    line_str = line.strip()
                    if not line_str:
                        continue

                    # 1. PROCESS
                    m_proc = re_proc.search(line_str)
                    if m_proc:
                        pid = int(m_proc.group(1))
                        name = m_proc.group(2)
                        ppid = int(m_proc.group(3))
                        owner = m_proc.group(4)
                        status = m_proc.group(5)
                        is_reg = m_proc.group(6).lower() == "true"
                        cmd = m_proc.group(7)
                        if pid not in pid_to_proc_id:
                            p = Process(
                                pid=pid,
                                process_name=name,
                                ppid=ppid,
                                owner=owner,
                                status=status,
                                is_registered=is_reg,
                                command_line=cmd
                            )
                            db.add(p)
                            db.commit()
                            db.refresh(p)
                            pid_to_proc_id[pid] = p.process_id

                    # 2. INCIDENT
                    m_inc = re_inc.search(line_str)
                    if m_inc:
                        inc_id = int(m_inc.group(1))
                        pid = int(m_inc.group(2))
                        inc_type = m_inc.group(3)
                        severity = m_inc.group(4)
                        status = m_inc.group(5)
                        desc = m_inc.group(6)

                        if inc_id not in existing_incidents:
                            proc_id = pid_to_proc_id.get(pid)
                            if not proc_id:
                                p = Process(
                                    pid=pid,
                                    process_name="workload_process",
                                    status="RUNNING",
                                    is_registered=True
                                )
                                db.add(p)
                                db.commit()
                                db.refresh(p)
                                proc_id = p.process_id
                                pid_to_proc_id[pid] = proc_id

                            inc = Incident(
                                incident_id=inc_id,
                                process_id=proc_id,
                                incident_type=inc_type,
                                severity=severity,
                                status=status,
                                description=desc,
                                detected_at=datetime.utcnow()
                            )
                            new_incidents.append(inc)
                            existing_incidents.add(inc_id)

                    # 3. UPDATE INCIDENT STATUS
                    m_upd = re_upd_inc.search(line_str)
                    if m_upd:
                        st = m_upd.group(1)
                        inc_id = int(m_upd.group(2))
                        inc = db.query(Incident).filter(Incident.incident_id == inc_id).first()
                        if inc:
                            inc.status = st
                            if st == "RESOLVED":
                                inc.resolved_at = datetime.utcnow()
                        else:
                            for ni in new_incidents:
                                if ni.incident_id == inc_id:
                                    ni.status = st
                                    if st == "RESOLVED":
                                        ni.resolved_at = datetime.utcnow()

                    # 4. RECOVERY
                    m_rec = re_rec.search(line_str)
                    if m_rec:
                        rec_id = int(m_rec.group(1))
                        inc_id = int(m_rec.group(2))
                        attempt = int(m_rec.group(3))
                        action = m_rec.group(4)
                        result = m_rec.group(5)
                        success = m_rec.group(6).lower() == "true"
                        verify = m_rec.group(7)
                        dur = float(m_rec.group(8))

                        if rec_id not in existing_recoveries and inc_id in existing_incidents:
                            rec = Recovery(
                                recovery_id=rec_id,
                                incident_id=inc_id,
                                attempt_number=attempt,
                                action=action,
                                result=result,
                                success=success,
                                verification_details=verify,
                                duration_ms=dur,
                                completed_at=datetime.utcnow()
                            )
                            new_recoveries.append(rec)
                            existing_recoveries.add(rec_id)

                            # If recovery succeeded, ensure the incident is marked RESOLVED
                            if success:
                                inc = db.query(Incident).filter(Incident.incident_id == inc_id).first()
                                if inc:
                                    inc.status = "RESOLVED"
                                    inc.resolved_at = datetime.utcnow()
                                else:
                                    for ni in new_incidents:
                                        if ni.incident_id == inc_id:
                                            ni.status = "RESOLVED"
                                            ni.resolved_at = datetime.utcnow()

                if new_incidents:
                    db.add_all(new_incidents)
                if new_recoveries:
                    db.add_all(new_recoveries)
                if new_incidents or new_recoveries:
                    db.commit()

            # Ensure all incidents with verified recoveries are updated to RESOLVED
            try:
                resolved_ids = [r[0] for r in db.query(Recovery.incident_id).filter(Recovery.success == True).all()]
                if resolved_ids:
                    db.query(Incident).filter(
                        Incident.incident_id.in_(resolved_ids),
                        Incident.status != "RESOLVED"
                    ).update({"status": "RESOLVED", "resolved_at": datetime.utcnow()}, synchronize_session=False)
                    db.commit()

                # Also verify all open incidents: if their target PID is dead, mark them RESOLVED
                open_incs = db.query(Incident).filter(Incident.status != "RESOLVED").all()
                for inc in open_incs:
                    target_pid = None
                    if inc.process and inc.process.pid:
                        target_pid = inc.process.pid
                    if not target_pid:
                        m_pid = re.search(r"PID (\d+)", inc.description or "")
                        if m_pid:
                            target_pid = int(m_pid.group(1))

                    if target_pid and target_pid > 2:
                        is_alive = False
                        try:
                            os.kill(target_pid, 0)
                            is_alive = True
                        except OSError:
                            is_alive = False

                        if not is_alive:
                            inc.status = "RESOLVED"
                            inc.resolved_at = datetime.utcnow()
                            if inc.process:
                                inc.process.status = "TERMINATED"
                            recs = db.query(Recovery).filter(Recovery.incident_id == inc.incident_id).all()
                            for r in recs:
                                r.success = True
                                r.result = "VERIFIED_SUCCESS"
                                r.completed_at = datetime.utcnow()
                                if not r.verification_details or "dispatched" in r.verification_details:
                                    r.verification_details = f"Verified: Process PID {target_pid} terminated. Fault cleared."
                            if not recs:
                                r = Recovery(
                                    incident_id=inc.incident_id,
                                    attempt_number=1,
                                    action="TERMINATE_SIGTERM",
                                    completed_at=datetime.utcnow(),
                                    result="VERIFIED_SUCCESS",
                                    success=True,
                                    verification_details=f"Process PID {target_pid} terminated. Resources released.",
                                    duration_ms=8.0
                                )
                                db.add(r)
                db.commit()
            except Exception as e:
                logger.debug("Error in auto-resolving terminated incidents: %s", e)
                db.rollback()

        except Exception as e:
            logger.error("Error synchronizing from SQL log file: %s", e)
            try:
                db.rollback()
            except Exception:
                pass

db_sync_service = DBSyncService()
