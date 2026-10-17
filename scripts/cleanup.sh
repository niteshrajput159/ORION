#!/usr/bin/env bash
# ==============================================================================
# OScope Clean Script
# Cleans build artifacts, temp sockets, logs, and caches.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "======================================================================"
echo "                     CLEANING OSCOPE ARTIFACTS                        "
echo "======================================================================"

# 1. Stop any running processes first
"${SCRIPT_DIR}/stop.sh" >/dev/null 2>&1 || true

# 2. Clean C++ build outputs
echo "Cleaning C++ build artifacts..."
rm -rf "${PROJECT_ROOT}/core/build"/*
rm -rf "${PROJECT_ROOT}/core/bin"/*
rm -f "${PROJECT_ROOT}/test_workloads/cpu_hog" \
      "${PROJECT_ROOT}/test_workloads/memory_hog" \
      "${PROJECT_ROOT}/test_workloads/crash_test" \
      "${PROJECT_ROOT}/test_workloads/crash_loop" \
      "${PROJECT_ROOT}/test_workloads/hang_test" \
      "${PROJECT_ROOT}/test_workloads/deadlock_test" \
      "${PROJECT_ROOT}/test_workloads/thread_hog" \
      "${PROJECT_ROOT}/test_workloads/io_hog" \
      "${PROJECT_ROOT}/test_workloads/fd_hog" \
      "${PROJECT_ROOT}/test_workloads/"*.o

# 3. Clean Python caches and logs
echo "Cleaning Python caches and system logs..."
find "${PROJECT_ROOT}" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find "${PROJECT_ROOT}" -type d -name ".pytest_cache" -exec rm -rf {} + 2>/dev/null || true
rm -f "${PROJECT_ROOT}/backend/"*.db
rm -f "${PROJECT_ROOT}/logs/"*.log
rm -f "${PROJECT_ROOT}/.pids"
rm -f "/tmp/oscope"*

# 4. Clean frontend build output
echo "Cleaning frontend dist output..."
rm -rf "${PROJECT_ROOT}/frontend/dist"

echo "Cleanup completed successfully."
echo "======================================================================"
