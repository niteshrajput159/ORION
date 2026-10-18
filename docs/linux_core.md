# OScope: Linux Systems Programming & Core Engine

The OScope Core (`core/`) is written in modern **C++17** and interacts directly with the Linux kernel via POSIX system calls and the `/proc` virtual filesystem.

---

## 1. Operating System Observation Engine

### Aggregate CPU Calculation via `/proc/stat`
Aggregate system CPU consumption is calculated by parsing the first line of `/proc/stat`:
```
cpu  user nice system idle iowait irq softirq steal guest guest_nice
```
1. Total Jiffies = $\sum \text{fields}(1..10)$
2. Idle Jiffies = $\text{idle} + \text{iowait}$
3. Active Jiffies = $\text{Total Jiffies} - \text{Idle Jiffies}$
4. Aggregate CPU Utilization:
$$\text{CPU}_{\%} = \frac{\Delta \text{Active Jiffies}}{\Delta \text{Total Jiffies}} \times 100\%$$

### Per-Process CPU Calculation via `/proc/[pid]/stat`
To obtain accurate per-process CPU percentages without sampling bias:
1. Field 14: `utime` (process user mode jiffies)
2. Field 15: `stime` (process kernel mode jiffies)
3. Total Process Ticks = `utime` + `stime`
4. Formula:
$$\text{Process CPU}_{\%} = \frac{\Delta \text{Process Ticks}}{\Delta \text{System Total Ticks}} \times \text{Number of Online Cores} \times 100\%$$

### Memory Footprint via `/proc/[pid]/statm` and `/proc/meminfo`
- **RSS (Resident Set Size)**: Read from field 2 of `/proc/[pid]/statm` multiplied by `sysconf(_SC_PAGESIZE)` (typically 4096 bytes).
- **VMS (Virtual Memory Size)**: Read from field 1 of `/proc/[pid]/statm`.
- **System Memory**: Parsed from `MemTotal`, `MemFree`, `MemAvailable`, and `Buffers` in `/proc/meminfo`.

### Open File Descriptors and Thread Counts
- **Thread Count**: Field 20 (`num_threads`) in `/proc/[pid]/stat` or by counting directory entries in `/proc/[pid]/task/`.
- **File Descriptors**: Scanned by opening `/proc/[pid]/fd/` and counting the number of symbolic link entries via `opendir`/`readdir`.

---

## 2. Process Control & POSIX Action Primitives

The C++ Process Controller executes real operating system interventions:

### 1. Priority Throttling (`setpriority`)
```cpp
// Throttles CPU hogs non-destructively
int new_nice = std::min(19, current_nice + 10);
setpriority(PRIO_PROCESS, pid, new_nice);
```

### 2. Graceful Termination (`SIGTERM`)
```cpp
// Issues graceful termination request
kill(pid, SIGTERM);
// Starts deadline timer (5.0s)
```

### 3. Forceful Preemption (`SIGKILL`)
```cpp
// Used when process is unresponsive to SIGTERM
kill(pid, SIGKILL);
```

### 4. Process Suspension & Resumption
```cpp
kill(pid, SIGSTOP); // Pauses thread scheduling
kill(pid, SIGCONT); // Resumes thread execution
```

### 5. Controlled Respawn (`fork` + `execvp`)
```cpp
pid_t child = fork();
if (child == 0) {
    // Child process: replace image
    char* const argv[] = { strdup(binary_path.c_str()), nullptr };
    execvp(binary_path.c_str(), argv);
    _exit(127);
}
```

---

## 3. Platform Abstraction & Dual-Target Design

OScope explicitly targets **Linux**. However, to enable local compilation, test running, and development on macOS developer workstations:
- Primary implementation directly reads and parses `/proc/stat`, `/proc/meminfo`, `/proc/[pid]/stat`.
- Dual-platform conditional compilation (`#ifdef __APPLE__`) provides fallback wrappers utilizing `sysctl` (`KERN_PROC`) and `proc_pidinfo` (`libproc.h`), guaranteeing clean builds without faking or mocking Linux semantics in production deployments.
- Build targets:
  - `core/Makefile` (Host compilation / Apple clang or GCC)
  - `core/Makefile.linux` (Strict Linux build with static libstdc++ options)
  - `Dockerfile.core` (Pure Ubuntu 22.04 LTS deployment container)
