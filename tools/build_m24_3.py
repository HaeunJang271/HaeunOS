#!/usr/bin/env python3
"""Build M24.3 init-reaper guest images."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_init_reaper() -> bytes:
    """Single/multi orphan + idle TRYWAIT image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_3_init_reaper.s")


def build_orphan_stress() -> bytes:
    """Repeated parent-before-child reclaim cycles."""
    return bpi.build_with_user(ROOT / "userland" / "m24_3_orphan_stress.s")


def build_reclamation() -> bytes:
    """AS capacity fill, WAIT reclaim, respawn."""
    return bpi.build_with_user(ROOT / "userland" / "m24_3_reclamation.s")
