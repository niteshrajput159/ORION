#!/usr/bin/env bash
# ==============================================================================
# OScope Stop Script
# Gracefully shuts down:
#   1. React Vite frontend server
#   2. FastAPI backend server
#   3. OScope C++ Core Daemon
#   4. Any active test workload processes
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PID_FILE="${PROJECT_ROOT}/.pids"

echo "======================================================================"
echo "                   STOPPING ORION SERVICES                           "
echo "======================================================================"

if [ -f "${PID_FILE}" ]; then
    while IFS='=' read -r NAME PID; do
        if [ -n "${PID}" ] && kill -0 "${PID}" 2>/dev/null; then
            echo "Stopping ${NAME} (PID: ${PID})..."
            kill -15 "${PID}" 2>/dev/null || true
            sleep 0.5
            if kill -0 "${PID}" 2>/dev/null; then
                echo "  Escalating to SIGKILL for ${NAME}..."
                kill -9 "${PID}" 2>/dev/null || true
            fi
        fi
    done < "${PID_FILE}"
    rm -f "${PID_FILE}"
fi

# Cleanup any stray instances
pkill -f "orion_daemon|oscope_daemon" 2>/dev/null || true
pkill -f "uvicorn app.main:app" 2>/dev/null || true
pkill -f "cpu_hog|memory_hog|crash_test|crash_loop|hang_test|deadlock_test|thread_hog|io_hog|fd_hog" 2>/dev/null || true

# Clean IPC socket and state files
rm -f "/tmp/orion.sock" "/tmp/orion_core.sock" "/tmp/oscope.sock" "/tmp/oscope_core.sock" "/tmp/oscope_deadlock.state"


echo "All Orion services and test workloads stopped cleanly."
echo "======================================================================"
