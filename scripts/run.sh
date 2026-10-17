#!/usr/bin/env bash
# ==============================================================================
# OScope Run Script
# Orchestrates launching:
#   1. OScope C++ Core Daemon (IPC socket /tmp/oscope.sock)
#   2. FastAPI Orchestration Backend (Uvicorn on :8000)
#   3. React Vite Dashboard Frontend (on :5173)
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PID_FILE="${PROJECT_ROOT}/.pids"
LOGS_DIR="${PROJECT_ROOT}/logs"

mkdir -p "${LOGS_DIR}"

echo "======================================================================"
echo "                   STARTING ORION SERVICES                          "
echo "======================================================================"

# Clean up stale socket or pids if left over
rm -f "/tmp/orion.sock" "/tmp/orion_core.sock" "/tmp/oscope.sock" "/tmp/oscope_core.sock" "/tmp/oscope_deadlock.state"
> "${PID_FILE}"

# 1. Start C++ Core Daemon
echo "[1/3] Launching Orion C++ Core Daemon..."
DAEMON_BIN=""
if [ -x "${PROJECT_ROOT}/core/bin/orion_daemon" ]; then
    DAEMON_BIN="${PROJECT_ROOT}/core/bin/orion_daemon"
elif [ -x "${PROJECT_ROOT}/core/bin/oscope_daemon" ]; then
    DAEMON_BIN="${PROJECT_ROOT}/core/bin/oscope_daemon"
fi

if [ -n "${DAEMON_BIN}" ]; then
    "${DAEMON_BIN}" > "${LOGS_DIR}/core.log" 2>&1 &
    CORE_PID=$!
    echo "CORE_PID=${CORE_PID}" >> "${PID_FILE}"
    echo "  -> Core Daemon running (PID: ${CORE_PID}, Logs: ${LOGS_DIR}/core.log)"
else
    echo "  ERROR: Daemon binary not found. Run ./scripts/build.sh first."
    exit 1
fi

sleep 1

# 2. Start FastAPI Backend
echo "[2/3] Launching FastAPI Backend on http://localhost:8000..."
# Check if venv exists
if [ -d "${PROJECT_ROOT}/.venv" ]; then
    source "${PROJECT_ROOT}/.venv/bin/activate"
fi

cd "${PROJECT_ROOT}/backend"
python3 -m uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload > "${LOGS_DIR}/backend.log" 2>&1 &
BACKEND_PID=$!
echo "BACKEND_PID=${BACKEND_PID}" >> "${PID_FILE}"
echo "  -> FastAPI running (PID: ${BACKEND_PID}, Dashboard: http://localhost:8000, Docs: http://localhost:8000/docs)"

sleep 1

# 3. Start Frontend Dashboard
echo "[3/3] Launching React Vite Frontend..."
if command -v npm >/dev/null 2>&1 && [ -f "${PROJECT_ROOT}/frontend/package.json" ]; then
    cd "${PROJECT_ROOT}/frontend"
    if [ -d "node_modules" ]; then
        npx vite --host 0.0.0.0 --port 5173 > "${LOGS_DIR}/frontend.log" 2>&1 &
        FRONTEND_PID=$!
        echo "FRONTEND_PID=${FRONTEND_PID}" >> "${PID_FILE}"
        echo "  -> Frontend UI running (PID: ${FRONTEND_PID}, Dashboard: http://localhost:5173)"
    else
        echo "  Notice: node_modules not yet installed in frontend/ (Run 'npm install' in frontend/ for standalone Vite UI)."
        echo "  Built-in Dashboard is already live and available at: http://localhost:8000"
    fi
else
    echo "  Notice: npm not found. Built-in Dashboard is live at: http://localhost:8000"
fi

echo "======================================================================"
echo "ORION SERVICES STARTED SUCCESSFULLY!"
echo "  Dashboard UI:      http://localhost:8000  (or http://127.0.0.1:8000)"
echo "  REST API Docs:     http://localhost:8000/docs"
if [ -n "${FRONTEND_PID}" ]; then
    echo "  React Vite UI:     http://localhost:5173"
fi
echo "  IPC Socket:        /tmp/orion.sock (or /tmp/oscope.sock)"
echo ""
echo "To stop services, execute: ./scripts/stop.sh"
echo "======================================================================"
