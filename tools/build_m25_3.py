#!/usr/bin/env python3
"""Build M25.3 FORK memory / isolation / failure guest images."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_snapshot() -> bytes:
    """Return HXF for stack+code snapshot across FORK."""
    return bpi.build_with_user(ROOT / "userland" / "m25_3_fork_snapshot.s")


def build_isolation() -> bytes:
    """Return HXF for parent/child stack write isolation."""
    return bpi.build_with_user(ROOT / "userland" / "m25_3_fork_isolation.s")


def build_failure() -> bytes:
    """Return HXF for AS-full FORK failure + parent preserve."""
    return bpi.build_with_user(ROOT / "userland" / "m25_3_fork_failure.s")
