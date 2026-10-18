"""
T9: Excessive File Descriptors Test Case
Verifies fd_hog opens large descriptor sets observed through /proc/[pid]/fd.
"""
import os
import subprocess
import time
import pytest
from app.services.core_client import core_client

def test_t9_excessive_file_descriptors():
    workload_bin = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../test_workloads/fd_hog"))
    assert os.path.exists(workload_bin), "fd_hog binary must be compiled"

    proc = subprocess.Popen([workload_bin, "50"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    pid = proc.pid
    core_client.register_process(pid, "fd_hog", f"{workload_bin} 50")

    time.sleep(0.8)
    info_res = core_client.get_process_info(pid)
    assert info_res.get("success") is True
    fds = info_res.get("data", {}).get("open_fds", 0)
    assert fds >= 10, "Open file descriptor count must reflect opened pipe handles"

    core_client.terminate_process(pid, "SIGTERM")
    proc.wait(timeout=2)
