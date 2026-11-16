#!/usr/bin/env python3
"""Build M25.4 stack guard / overflow guest images."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build_boundary() -> bytes:
    """Return HXF for valid edges + guard-fault containment."""
    return bpi.build_with_user(ROOT / "userland" / "m25_4_stack_boundary.s")


def build_overflow() -> bytes:
    """Return HXF for deep stack overflow in a child."""
    return bpi.build_with_user(ROOT / "userland" / "m25_4_stack_overflow.s")


def build_fork_guard() -> bytes:
    """Return HXF for FORK stack guard isolation and slot reuse."""
    return bpi.build_with_user(ROOT / "userland" / "m25_4_fork_stack_guard.s")
