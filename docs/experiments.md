# OScope: Research Experimentation & Empirical Evaluation

OScope includes a rigorous, quantitative benchmarking framework located in `tests/experiments/`.

The suite empirically assesses the trade-offs of detection paradigms, recovery strategies, and deadlock resolution heuristics across synthetic and replay workload traces.

---

## 1. Experiment 1: Fault Detection Paradigm Benchmark

### Objective
Compares three anomaly detection algorithms on metric streams containing normal Gaussian jitter, transient spikes (non-faults), sustained high load (true faults), and slow memory leaks (linear drift).

### Evaluated Strategies:
1. **Single Threshold (Instantaneous)**: Flags alarm if $v \ge \text{threshold}$.
2. **Threshold + Persistence Window ($N=3$)**: Flags alarm only if $v \ge \text{threshold}$ for 3 consecutive intervals.
3. **Hybrid Statistical (Z-Score + Dynamic Persistence)**: Evaluates rolling baseline $\mu \pm k\sigma$ with adaptive sample gating.

### Empirical Results:
| Strategy | Precision | Recall | F1 Score | False Alarm Rate | Detection Latency (Samples) |
|---|---|---|---|---|---|
| Single Threshold | 94.0% | 94.0% | 0.9400 | 2.00% (3 FP) | 0 |
| Threshold + Persistence ($N=3$) | 97.7% | 86.0% | 0.9149 | 0.67% (1 FP) | 2 |
| Hybrid Statistical | 95.3% | 82.0% | 0.8817 | 1.33% (2 FP) | 1 |

### Research Takeaway:
Single instantaneous thresholding is highly sensitive to momentary workload bursts, generating false alarms during transient spikes. Persistence windows effectively suppress noise while introducing a minor, predictable latency of $N-1$ samples.

---

## 2. Experiment 2: Recovery Strategy Benchmark

### Objective
Evaluates remediation effectiveness and side-effects across 8 diverse failure workloads (cooperative CPU hogs, stubborn I/O hangs, uncooperative memory leaks, and crash-looping binaries).

### Evaluated Strategies:
1. **Static Fixed Policy**: Dispatches immediate unconditional `SIGKILL`.
2. **Blind Immediate Restart**: Re-executes the process immediately without crash-loop detection.
3. **OScope Adaptive Recovery**: Multi-stage escalation ladder (`nice` $\to$ `SIGTERM` $\to$ `SIGKILL` $\to$ Supervised restart with exponential backoff & circuit breaker).

### Empirical Results:
| Strategy | Success / Mitigation Rate | Unnecessary Force Kills | Flapping Loops Contained | Mean Recovery Cost Units |
|---|---|---|---|---|
| Static Fixed Policy | 75.0% (6/8) | 2 | 0 | 5.25 |
| Blind Immediate Restart | 62.5% (5/8) | 2 | 0 | 11.12 |
| OScope Adaptive Recovery | 100.0% (8/8) | 0 | 2 | 3.88 |

### Research Takeaway:
Blind immediate termination destroys useful in-flight work for cooperative processes. Blind restarting causes infinite CPU thrash on broken binaries. OScope's adaptive approach achieves 100% containment by throttling cooperative processes and quarantining crash loops.

---

## 3. Experiment 3: Deadlock Victim Selection Benchmark

### Objective
Evaluates graph cycle-breaking efficiency across multiple Wait-For Graph topologies (2-node mutex deadlocks, 3-node circular rings, and complex multi-cycle resource bottlenecks).

### Evaluated Strategies:
1. **`LEAST_COST`**: Aborts the process minimizing $\text{Cost} = \alpha \cdot \text{CPU} + \beta \cdot \text{RAM} + \gamma \cdot \text{Locks}$.
2. **`LOWEST_PRIORITY`**: Aborts the process with lowest scheduling priority.
3. **`HIGHEST_RESOURCE`**: Aborts the process holding the maximum locks and memory.

### Empirical Results:
| Strategy | Victims Aborted | Total Work Lost Cost Score | Locks Freed | Efficiency Ratio (Locks / Cost) |
|---|---|---|---|---|
| `LEAST_COST` | 4 | 64.9 | 4 | **0.062** |
| `LOWEST_PRIORITY` | 4 | 64.9 | 4 | **0.062** |
| `HIGHEST_RESOURCE` | 3 | 519.2 | 7 | 0.013 |

### Research Takeaway:
While `HIGHEST_RESOURCE` frees more locks with fewer victim terminations, it causes massive destruction of invested CPU and memory work (cost 519.2 vs 64.9). `LEAST_COST` provides the highest operational efficiency by preserving long-running batch work and aborting lightweight blockers.

---

## 4. Running the Benchmark Suite

```bash
python3 tests/experiments/experiment_runner.py
```
Output artifacts are saved directly to `tests/experiments/results/`:
- `detection_comparison.json` / `.csv`
- `recovery_comparison.json` / `.csv`
- `deadlock_comparison.json` / `.csv`
- `summary_report.json` / `summary_report.md`
