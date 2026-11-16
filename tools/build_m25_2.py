#!/usr/bin/env python3
"""Build M25.2 FORK policy guest image."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_fork_policy() -> bytes:
    """Return HXF exercising FORK code-page copy + WAIT."""
    return bpi.build_with_user(ROOT / "userland" / "m25_2_fork_policy.s")
