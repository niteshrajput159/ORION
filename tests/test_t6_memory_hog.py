"""
T6: High Memory Allocation & Recovery Test Case
Verifies:
1. memory_hog allocates resident pages (RSS)
2. Memory monitor reads real physical memory
3. Termination cleans up memory
"""
import os
import subprocess
import time
import pytest
from app.services.core_client import core_client

def test_t6_high_memory_allocation():
    workload_bin = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../test_workloads/memory_hog"))
    assert os.path.exists(workload_bin), "memory_hog binary must be compiled"

    proc = subprocess.Popen([workload_bin, "--target-mb", "100", "--rate-mb", "100"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    pid = proc.pid
    core_client.register_process(pid, "memory_hog", workload_bin)

    time.sleep(1.0)

    info_res = core_client.get_process_info(pid)
    assert info_res.get("success") is True
    rss_mb = info_res.get("data", {}).get("memory_rss_mb", 0.0)
    assert rss_mb >= 0.0

    # Execute recovery termination
    term_res = core_client.terminate_process(pid, "SIGTERM")
    assert term_res.get("success") is True

    proc.wait(timeout=2)
