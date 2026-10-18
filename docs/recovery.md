# OScope: Automated Recovery Engine & Adaptive Escalation

OScope implements an **Adaptive Recovery Engine** that replaces naive, blind crash loops or immediate unconditional kills with an intelligent, history-aware remediation ladder.

---

## 1. The Multi-Stage Escalation Ladder

Rather than immediately terminating every aberrant process, OScope selects the least disruptive intervention capable of resolving the fault:

```
[Fault Detected]
       |
       v
+-------------------------------+
| Tier 1: Non-Destructive       |  -> CHANGE_PRIORITY (+10 Nice)
| Throttling                    |     CPU hogs are lowered in priority
+---------------+---------------+
                | Unresolved / Ineffective
                v
+-------------------------------+
| Tier 2: Graceful Termination  |  -> SIGTERM (POSIX Signal 15)
| with Deadline (5.0s)          |     Process given 5s to flush state
+---------------+---------------+
                | Unresponsive / Stubborn Hang
                v
+-------------------------------+
| Tier 3: Unconditional Kill    |  -> SIGKILL (POSIX Signal 9)
|                               |     Immediate preemption by kernel
+---------------+---------------+
                | If service restart policy is enabled
                v
+-------------------------------+
| Tier 4: Supervised Respawn    |  -> RESTART (fork + execvp)
| with Exponential Backoff      |     With Crash-Loop Circuit Breaker
+-------------------------------+
```

---

## 2. Crash-Loop Detection & Circuit Breaker

Blind restarts can cause catastrophic runaway CPU and I/O thrashing if a misconfigured or corrupted service crashes repeatedly on boot.

### The Algorithm:
1. When a process terminates abnormally (exit code $\neq 0$ or terminated by signal), the event is recorded in the binary's restart history with a microsecond timestamp.
2. The engine evaluates the sliding time window ($W = 30$ seconds).
3. If the number of crashes $C \ge 3$ within window $W$:
   - The engine flags a **CRASH_LOOP** incident.
   - **Circuit Breaker Triggers**: Automated restarts are immediately suspended for this binary.
   - Process is quarantined.
   - High-priority operator alert and `RESOURCE_EVENT` are emitted to PostgreSQL.
4. If $C < 3$:
   - Exponential backoff delay is applied:
$$\text{Delay} = \text{base\_delay} \times 2^{\text{attempt}} \quad (1\text{s}, 2\text{s}, 4\text{s})$$
   - Binary is respawned under supervision.

---

## 3. Verification Engine

No recovery action is assumed to be successful until verified through active OS telemetry:

| Action Executed | Verification Procedure | Success Condition |
|---|---|---|
| `CHANGE_PRIORITY` | Sample process CPU usage over subsequent 3 polling ticks | $\text{CPU}_{\%} < \text{Threshold}$ |
| `TERMINATE_SIGTERM` | Poll `/proc/[pid]` every 250ms up to 5.0 seconds | Process PID disappears from `/proc` |
| `TERMINATE_SIGKILL` | Poll `kill(pid, 0)` or `/proc/[pid]` | `ESRCH` (No such process) confirmed |
| `RESTART` | Confirm new child PID exists and maintains running state for $\ge 3\text{s}$ | Process stays alive in state `R` or `S` |
| `RESOLVE_DEADLOCK` | Re-run cycle detection on Wait-For Graph | Number of directed cycles drops to $0$ |

If verification fails before the timeout expires, the action is marked as `FAILED` in the `RECOVERY` table, and the engine automatically triggers the next tier in the escalation ladder.
