#!/usr/bin/env python3
"""Build M25.6 heap guest images (with haeun_malloc.s)."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]
MALLOC = ROOT / "userland" / "haeun_malloc.s"


def _with_malloc(user: str) -> bytes:
    """Link userland test with the fixed-arena allocator."""
    return bpi.build_with_user(ROOT / "userland" / user, [MALLOC])


def build_mapping() -> bytes:
    """Heap store/load smoke on init (no allocator link required)."""
    return bpi.build_with_user(ROOT / "userland" / "m25_6_heap_mapping.s")


def build_allocator() -> bytes:
    """malloc/free/calloc/realloc / OOM markers."""
    return _with_malloc("m25_6_allocator.s")


def build_isolation() -> bytes:
    """FORK heap clone + independence."""
    return _with_malloc("m25_6_heap_isolation.s")


def build_lifecycle() -> bytes:
    """Child EXIT; parent heap intact; reuse FORK."""
    return _with_malloc("m25_6_heap_lifecycle.s")
