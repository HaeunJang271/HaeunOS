#!/usr/bin/env python3
"""Verify SHA256SUMS against files in the same package root."""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    """Return hex SHA-256 of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main(argv: list[str]) -> int:
    """Exit 0 on match; 1 on mismatch/missing; 2 on usage."""
    root = Path(argv[1]).resolve() if len(argv) > 1 else Path(__file__).resolve().parents[1]
    sums = root / "SHA256SUMS"
    if not sums.is_file():
        # When invoked from package scripts/, parent is package root.
        if (root.parent / "SHA256SUMS").is_file():
            root = root.parent
            sums = root / "SHA256SUMS"
        else:
            print(f"missing SHA256SUMS under {root}", file=sys.stderr)
            return 2
    bad = 0
    for line in sums.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        digest, _, rel = line.partition("  ")
        if not rel:
            print(f"bad SUMS line: {line}", file=sys.stderr)
            bad += 1
            continue
        path = root / rel
        if not path.is_file():
            print(f"MISSING {rel}", file=sys.stderr)
            bad += 1
            continue
        got = sha256_file(path)
        if got != digest:
            print(f"MISMATCH {rel}\n  want {digest}\n  got  {got}", file=sys.stderr)
            bad += 1
        else:
            print(f"ok {rel}")
    if bad:
        print(f"verify failed: {bad} problem(s)", file=sys.stderr)
        return 1
    print("SHA256SUMS verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
