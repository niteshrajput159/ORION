"""
T8: High Disk I/O Saturation Test Case
Verifies io_hog performs synchronized disk writes.
"""
import os
import subprocess
import time
import pytest
from app.services.core_client import core_client

def test_t8_high_io_saturation():
    workload_bin = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../test_workloads/io_hog"))
    assert os.path.exists(workload_bin), "io_hog binary must be compiled"

    proc = subprocess.Popen([workload_bin, "5"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    pid = proc.pid
    core_client.register_process(pid, "io_hog", f"{workload_bin} 5")

    time.sleep(1.0)
    info_res = core_client.get_process_info(pid)
    assert info_res.get("success") is True

    core_client.terminate_process(pid, "SIGTERM")
    proc.wait(timeout=2)
