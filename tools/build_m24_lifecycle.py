#!/usr/bin/env python3
"""Build M24.2 guest process images for lifecycle hardening tests."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_lifecycle() -> bytes:
    """Return HXF for FORK/WAIT/EXIT contract image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_lifecycle.s")


def build_orphan_wait() -> bytes:
    """Return HXF for child-before/after WAIT image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_orphan_wait.s")


def build_parent_death() -> bytes:
    """Return HXF for middle-exit orphan reparent image."""
    return bpi.build_with_user(ROOT / "userland" / "m24_parent_death.s")
