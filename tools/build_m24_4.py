#!/usr/bin/env python3
"""Build M24.4 lifecycle stress guest images."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_lifecycle() -> bytes:
    """WAIT/TRYWAIT interaction and orphan cycle image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_4_lifecycle.s")


def build_pid_reuse() -> bytes:
    """PID reuse and foreign-zombie protection image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_4_pid_reuse.s")
