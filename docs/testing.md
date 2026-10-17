# OScope: Test Engineering & Quality Assurance

OScope incorporates a multi-tiered verification strategy spanning C++ unit tests, fault-injection binaries, integration workflows, and API regression suites.

---

## 1. Test Architecture Matrix

```
+-------------------------------------------------------------------------+
| Level 4: System Integration (tests/integration/test_full_pipeline.py)    |
|   End-to-End Observation -> Detection -> Action -> Verification         |
+-------------------------------------------------------------------------+
| Level 3: System Fault Injection (tests/system/test_t1_crash.py - t10)   |
|   Real process spawning, kernel signal dispatch, cycle breaking         |
+-------------------------------------------------------------------------+
| Level 2: Backend API & Service Tests (backend/tests/)                   |
|   Route contract verification, Pydantic validations, Whitelisting      |
+-------------------------------------------------------------------------+
| Level 1: C++ Algorithmic Unit Tests (core/tests/run_core_tests.cpp)     |
|   Z-Score, WFG DFS cycles, Victim Cost heuristics, Crash-loop window    |
+-------------------------------------------------------------------------+
```

---

## 2. C++ Algorithmic Unit Test Suite (`core/bin/run_core_tests`)

Compiled from `core/tests/run_core_tests.cpp`:
1. `test_threshold_detection`: Verifies instantaneous threshold crossing logic.
2. `test_persistence_detection`: Verifies that breaches trigger only after $N$ consecutive samples.
3. `test_statistical_detection`: Verifies rolling Z-Score mean/variance calculations.
4. `test_deadlock_cycle_detection`: Validates 3-color DFS cycle discovery on 2-node and 3-node graphs.
5. `test_victim_selection`: Verifies that `LEAST_COST` minimizes wasted work.
6. `test_crash_loop_detection`: Validates sliding window retry limits and circuit breaker trip.
7. `test_adaptive_recovery`: Verifies multi-stage escalation logic.
8. `test_verification_engine`: Verifies post-action polling verification logic.

**Execution:**
```bash
./core/bin/run_core_tests
```

---

## 3. Real C++ Fault-Injection Test Workloads (`test_workloads/`)

Each workload is an authentic, compiled C++ program designed to inject specific operating system anomalies:

| Workload Binary | Source Code | Failure Mode Simulated | Verification Expected |
|---|---|---|---|
| `cpu_hog` | `cpu_hog.cpp` | Spins active loop across CPU cores ($>95\%$ CPU) | Nice throttling or SIGTERM reduces usage |
| `memory_hog` | `memory_hog.cpp` | Allocates and touches 50MB blocks rapidly | Memory threshold breached, SIGTERM sent |
| `crash_test` | `crash_test.cpp` | Dereferences null pointer (`*nullptr = 42`) generating `SIGSEGV` | Process crash logged in DB, auto-restarted |
| `crash_loop` | `crash_loop.cpp` | Exits with failure code immediately after starting | 3 rapid crashes detected, circuit breaker halts restarts |
| `hang_test` | `hang_test.cpp` | Traps `SIGTERM` and sleeps indefinitely | SIGTERM times out, escalates to `SIGKILL` |
| `deadlock_test` | `deadlock_test.cpp` | 2 pthreads acquire `mutex_a` and `mutex_b` in inverted order | WFG cycle discovered, victim chosen and aborted |
| `thread_hog` | `thread_hog.cpp` | Spawns 120 spinning pthreads | Thread threshold breached, process mitigated |
| `io_hog` | `io_hog.cpp` | Flushes unbuffered random disk writes via `fsync` | I/O threshold flagged |
| `fd_hog` | `fd_hog.cpp` | Opens 300 persistent `/tmp` file descriptors | File descriptor leak flagged |

**Compilation:**
```bash
make -C test_workloads all
```

---

## 4. System Test Suite (`tests/system/`)

A comprehensive suite of 10 automated end-to-end integration tests:
- `test_t1_crash.py`: Validates detection and restart of single crashing process.
- `test_t2_crash_loop.py`: Validates that flapping crashes trip the circuit breaker.
- `test_t3_hang.py`: Validates escalation from SIGTERM to SIGKILL on unresponsive processes.
- `test_t4_deadlock.py`: Validates WFG graph generation, cycle finding, and victim preemption.
- `test_t5_cpu_hog.py`: Validates non-destructive CPU throttling via renice.
- `test_t6_memory_hog.py`: Validates RSS memory leak detection.
- `test_t7_threads.py`: Validates excessive thread spawning containment.
- `test_t8_io.py`: Validates continuous disk I/O thrashing detection.
- `test_t9_fd.py`: Validates file descriptor leak containment.
- `test_t10_normal_exit.py`: Confirms normal process termination does NOT trigger false incidents.
- `test_failure_modes.py`: Validates resilience against socket disconnects and DB timeouts.

**Run All Tests:**
```bash
./scripts/run_tests.sh
```
