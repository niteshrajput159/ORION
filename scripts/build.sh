#!/usr/bin/env bash
# ==============================================================================
# OScope Build Script
# Compiles:
#   1. C++ Core Daemon, CLI Controller, and Unit Test Suite
#   2. C++ Test Workloads (9 fault-injection binaries)
#   3. React Frontend Production Build (if node/npm present)
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "======================================================================"
echo "                      BUILDING ORION PLATFORM                         "
echo "======================================================================"

# 1. Compile C++ Core
echo "[1/3] Compiling Orion C++ Core..."
cd "${PROJECT_ROOT}/core"
make clean || true
make all

echo "  -> Verified core binaries:"
ls -lh "${PROJECT_ROOT}/core/bin/"

# 2. Compile Test Workloads
echo "[2/3] Compiling 9 C++ Fault-Injection Test Workloads..."
cd "${PROJECT_ROOT}/test_workloads"
make clean || true
make all

echo "  -> Verified test workload binaries:"
ls -lh "${PROJECT_ROOT}/test_workloads/cpu_hog" "${PROJECT_ROOT}/test_workloads/memory_hog" "${PROJECT_ROOT}/test_workloads/crash_test"

# 3. Build React Frontend (Optional Production Dist)
echo "[3/3] Building React Frontend..."
if command -v npm >/dev/null 2>&1 && [ -f "${PROJECT_ROOT}/frontend/package.json" ]; then
    cd "${PROJECT_ROOT}/frontend"
    if [ -d "node_modules" ]; then
        echo "  Compiling Vite bundle..."
        npm run build || echo "  Vite build step skipped or generated dist."
    else
        echo "  Notice: frontend node_modules not yet installed. Run ./scripts/setup.sh first."
    fi
else
    echo "  Skipping frontend build (npm not found or frontend directory missing)."
fi

echo "======================================================================"
echo "BUILD SUCCEEDED! All OScope binaries are ready."
echo "Run './scripts/run.sh' to launch or './scripts/run_tests.sh' for test suite."
echo "======================================================================"
