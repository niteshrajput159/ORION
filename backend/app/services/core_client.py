import socket
import json
import logging
import subprocess
import os
from typing import Dict, Any, Optional
from app.config import settings

logger = logging.getLogger("oscope.core_client")

class CoreClient:
    def __init__(self, socket_path: Optional[str] = None):
        self.socket_path = socket_path or settings.OSCOPE_IPC_SOCKET
        orion_ctl = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../core/bin/orion_ctl"))
        oscope_ctl = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../core/bin/oscope_ctl"))
        self.ctl_path = orion_ctl if os.path.exists(orion_ctl) else oscope_ctl

    def send_command(self, command_line: str) -> Dict[str, Any]:
        """
        Sends command to OScope C++ Core via UNIX domain socket.
        Falls back to executing oscope_ctl binary if socket is unavailable.
        """
        # Try UNIX Domain Socket
        if os.path.exists(self.socket_path):
            try:
                client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                client.settimeout(2.0)
                client.connect(self.socket_path)
                client.sendall((command_line + "\n").encode("utf-8"))
                
                response_data = b""
                while True:
                    chunk = client.recv(4096)
                    if not chunk:
                        break
                    response_data += chunk
                client.close()
                
                if response_data:
                    return json.loads(response_data.decode("utf-8"))
            except Exception as e:
                logger.debug("Socket communication failed (%s), falling back to ctl binary.", e)

        # Fallback: Execute oscope_ctl directly
        if os.path.exists(self.ctl_path):
            try:
                args = [self.ctl_path] + command_line.split()
                res = subprocess.run(args, capture_output=True, text=True, timeout=5)
                if res.stdout:
                    return json.loads(res.stdout.strip())
            except Exception as e:
                logger.error("Failed executing oscope_ctl (%s)", e)

        # Standalone mock/fallback for offline dev
        return {"success": False, "error": "OScope Core daemon and CLI unavailable", "data": None}

    def get_system_health(self) -> Dict[str, Any]:
        return self.send_command("GET_SYSTEM_HEALTH")

    def get_process_info(self, pid: int) -> Dict[str, Any]:
        return self.send_command(f"GET_PROCESS_INFO {pid}")

    def terminate_process(self, pid: int, mode: str = "SIGTERM") -> Dict[str, Any]:
        return self.send_command(f"TERMINATE_PROCESS {pid} {mode}")

    def suspend_process(self, pid: int) -> Dict[str, Any]:
        return self.send_command(f"SUSPEND_PROCESS {pid}")

    def resume_process(self, pid: int) -> Dict[str, Any]:
        return self.send_command(f"RESUME_PROCESS {pid}")

    def change_priority(self, pid: int, nice_value: int) -> Dict[str, Any]:
        return self.send_command(f"CHANGE_PRIORITY {pid} {nice_value}")

    def restart_process(self, pid: int, cmd_line: str = "") -> Dict[str, Any]:
        return self.send_command(f"RESTART_PROCESS {pid} {cmd_line}")

    def verify_process(self, pid: int) -> Dict[str, Any]:
        return self.send_command(f"VERIFY_PROCESS {pid}")

    def check_deadlock(self, state_file: str = "/tmp/oscope_deadlock.state") -> Dict[str, Any]:
        return self.send_command(f"CHECK_DEADLOCK {state_file}")

    def register_process(self, pid: int, name: str, cmd_line: str = "") -> Dict[str, Any]:
        return self.send_command(f"REGISTER_PROCESS {pid} {name} {cmd_line}")

    def unregister_process(self, pid: int) -> Dict[str, Any]:
        return self.send_command(f"UNREGISTER_PROCESS {pid}")

core_client = CoreClient()
