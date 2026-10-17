#!/usr/bin/env bash
# ==============================================================================
# OScope Environment Setup Script
# Checks system dependencies, creates virtualenv, installs Python & JS packages,
# and prepares the build environment.
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "======================================================================"
echo "                   OSCOPE SETUP & INITIALIZATION                     "
echo "======================================================================"
echo "Project root: ${PROJECT_ROOT}"

# 1. Check C++ Compiler
echo -n "[1/6] Checking C++ compiler... "
if command -v clang++ >/dev/null 2>&1; then
    CXX_COMPILER="clang++"
elif command -v g++ >/dev/null 2>&1; then
    CXX_COMPILER="g++"
else
    echo "ERROR: Neither clang++ nor g++ found. Please install a C++17 compiler."
    exit 1
fi
echo "Found (${CXX_COMPILER})"

# 2. Check Python 3
echo -n "[2/6] Checking Python 3... "
if command -v python3 >/dev/null 2>&1; then
    PY_VER=$(python3 --version)
    echo "Found (${PY_VER})"
else
    echo "ERROR: python3 not found. Please install Python 3.9+."
    exit 1
fi

# 3. Check Node.js and npm
echo -n "[3/6] Checking Node.js & npm... "
if command -v node >/dev/null 2>&1 && command -v npm >/dev/null 2>&1; then
    NODE_VER=$(node --version)
    NPM_VER=$(npm --version)
    echo "Found (Node ${NODE_VER}, npm ${NPM_VER})"
else
    echo "WARNING: Node.js / npm not detected. Frontend build requires Node 18+."
fi

# 4. Prepare Environment & Configurations
echo "[4/6] Initializing directories and configurations..."
mkdir -p "${PROJECT_ROOT}/logs"
mkdir -p "${PROJECT_ROOT}/data"
mkdir -p "${PROJECT_ROOT}/core/bin"
mkdir -p "${PROJECT_ROOT}/core/build"

if [ ! -f "${PROJECT_ROOT}/.env" ]; then
    if [ -f "${PROJECT_ROOT}/.env.example" ]; then
        cp "${PROJECT_ROOT}/.env.example" "${PROJECT_ROOT}/.env"
        echo "  Created .env from .env.example"
    fi
fi

# 5. Set up Python Environment & Dependencies
echo "[5/6] Setting up Python dependencies..."
cd "${PROJECT_ROOT}"

if [ ! -d ".venv" ]; then
    echo "  Creating virtual environment in .venv..."
    python3 -m venv .venv
fi

# Activate venv
source .venv/bin/activate
echo "  Upgrading pip..."
pip install --quiet --upgrade pip

if [ -f "backend/requirements.txt" ]; then
    echo "  Installing backend/requirements.txt..."
    pip install --quiet -r backend/requirements.txt || {
        echo "  Notice: pip install encountered partial constraint. Installing core packages..."
        pip install --quiet fastapi uvicorn sqlalchemy psycopg2-binary pydantic pyyaml httpx pytest
    }
fi

# 6. Frontend Dependencies
echo "[6/6] Setting up Frontend dependencies..."
if command -v npm >/dev/null 2>&1 && [ -f "${PROJECT_ROOT}/frontend/package.json" ]; then
    cd "${PROJECT_ROOT}/frontend"
    echo "  Running npm install in frontend/..."
    npm install --quiet --no-audit --prefer-offline 2>/dev/null || npm install --quiet
fi

echo "======================================================================"
echo "OSCOPE SETUP COMPLETE! Next step: ./scripts/build.sh"
echo "======================================================================"
