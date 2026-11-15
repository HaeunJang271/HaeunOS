#!/usr/bin/env python3
"""M13: guest net_echo through --net-real with a local UDP echo server."""
from __future__ import annotations

import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_net_echo  # noqa: E402

ECHO_PORT = 19007


def _echo_server(stop: threading.Event) -> None:
    """Reflect UDP payloads on 127.0.0.1:ECHO_PORT until stop is set."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("127.0.0.1", ECHO_PORT))
    sock.settimeout(0.05)
    while not stop.is_set():
        try:
            data, addr = sock.recvfrom(512)
        except socket.timeout:
            continue
        except OSError:
            break
        try:
            sock.sendto(data, addr)
        except OSError:
            break
    sock.close()


def main(argv: list[str]) -> int:
    """Build process_net_echo.hxf, serve UDP echo, run haeun-vm --net-real."""
    if len(argv) != 2:
        print("usage: m13_process_net_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "process_net_echo.hxf"
    image.write_bytes(build_process_net_echo.build())

    stop = threading.Event()
    thr = threading.Thread(target=_echo_server, args=(stop,), daemon=True)
    thr.start()
    time.sleep(0.05)
    try:
        result = subprocess.run(
            [str(vm), str(image), "--net-real"],
            capture_output=True,
            text=True,
            check=False,
            timeout=60,
        )
    finally:
        stop.set()
        thr.join(timeout=2.0)

    if result.returncode != 0:
        print(result.stdout, result.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in result.stdout, result.stdout
    assert "r1=0x00000001" in result.stdout, result.stdout
    assert "real=1" in result.stdout, result.stdout
    assert "be_tx=" in result.stdout, result.stdout
    assert "be_rx=" in result.stdout, result.stdout
    assert "real RX inject" in result.stdout, result.stdout
    print("m13 process net tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
