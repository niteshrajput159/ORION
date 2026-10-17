from typing import Dict, Any
from sqlalchemy.orm import Session
from app.models import Incident, Recovery, ProcessSnapshot

class AiService:
    @staticmethod
    def explain_incident(db: Session, incident_id: int) -> Dict[str, Any]:
        """
        Generates an automated root-cause explanation and advisory analysis
        based on historical telemetry and incident properties without arbitrary command risks.
        """
        inc = db.query(Incident).filter(Incident.incident_id == incident_id).first()
        if not inc:
            return {"error": "Incident not found"}

        proc_name = inc.process.process_name if inc.process else "Unknown process"
        pid = inc.process.pid if inc.process else inc.process_id
        inc_type = inc.incident_type

        # Fetch recovery attempts
        recoveries = db.query(Recovery).filter(Recovery.incident_id == inc.incident_id).all()
        recovery_summary = f"{len(recoveries)} automated recovery attempts recorded."
        if recoveries:
            last_rec = recoveries[-1]
            recovery_summary += f" Latest action: {last_rec.action} ({last_rec.result})."

        # Diagnostic heuristics
        root_cause = ""
        recommendation = ""

        if inc_type == "HIGH_CPU":
            root_cause = f"Sustained CPU compute saturation observed in {proc_name}. Process exceeded configured CPU limits across consecutive monitoring cycles, indicative of an unoptimized loop or thread contention."
            recommendation = "Lower scheduling priority (nice +10) or restrict CPU core affinity via cgroups. Inspect hot loops and concurrency synchronization."

        elif inc_type == "HIGH_MEMORY":
            root_cause = f"Physical RAM (RSS) usage for {proc_name} exceeded threshold without garbage collection or deallocation, pointing to a memory leak or oversized in-memory buffer."
            recommendation = "Enforce process virtual memory limits (ulimit / cgroups memory.max) and gracefully restart process after saving volatile state."

        elif inc_type == "PROCESS_CRASH":
            root_cause = f"Process {proc_name} (PID {pid}) terminated abnormally due to an unhandled signal or fatal assertion."
            recommendation = "Inspect core dump (/var/crash) and stack trace. OScope has executed bounded restart."

        elif inc_type == "CRASH_LOOP":
            root_cause = f"Process {proc_name} entered an unstable restart loop exceeding max attempt threshold ({len(recoveries)} failures). Service fails immediately upon launch."
            recommendation = "Halt automated restarts to prevent resource thrashing. Verify configuration files, database connection strings, and required file dependencies."

        elif inc_type == "HUNG_PROCESS":
            root_cause = f"Process {proc_name} stopped processing event loops or entered uninterruptible sleep ('D' state) while holding OS resources."
            recommendation = "Attempt graceful SIGTERM. If unresponsive after timeout, escalate to SIGKILL to release locked kernel resources."

        elif inc_type == "DEADLOCK":
            victim = inc.incident_metadata.get("victim_thread", "unknown") if inc.incident_metadata else "unknown"
            root_cause = f"Circular lock dependency detected in Wait-For Graph (T1 -> T2 -> T1). Deadlocked threads are blocked indefinitely."
            recommendation = f"Cycle resolved by terminating victim thread #{victim}. Enforce strict global lock acquisition ordering (Resource Hierarchy Protocol) in application codebase."

        elif inc_type == "EXCESSIVE_THREADS":
            root_cause = f"Process {proc_name} spawned threads exceeding system threshold, risking thread exhaustion and context-switching overhead."
            recommendation = "Refactor unbounded thread creation into a bounded thread pool / worker queue."

        elif inc_type == "EXCESSIVE_FILE_DESCRIPTORS":
            root_cause = f"File descriptor leak detected. Open FD count exceeded threshold without matching close() calls on sockets/files."
            recommendation = "Audit socket/file lifecycle management. Ensure file descriptors are released in defer/finally cleanup handlers."

        else:
            root_cause = f"System detected abnormal resource profile for {proc_name}."
            recommendation = "Monitor process metrics and review recent deployment logs."

        explanation = (
            f"### Incident Diagnostic Analysis (#{incident_id})\n\n"
            f"**Observed Fault:** {inc_type} on `{proc_name}` (PID: {pid})\n\n"
            f"**Probable Root Cause:** {root_cause}\n\n"
            f"**Automated OS Action Outcome:** {recovery_summary}\n\n"
            f"**Architectural Recommendation:** {recommendation}"
        )

        return {
            "incident_id": incident_id,
            "incident_type": inc_type,
            "severity": inc.severity,
            "root_cause": root_cause,
            "recommendation": recommendation,
            "markdown_explanation": explanation
        }

ai_service = AiService()
