#!/usr/bin/env python3
"""M24.3: AS capacity recovery via WAIT; TRYWAIT empty; SPAWN fallback retained."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_3 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Fill AS, reap, respawn; document SPAWN memset still present in app_env."""
    if len(argv) != 2:
        print("usage: m24_3_reclamation_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    src = (ROOT / "src" / "app_env.c").read_text(encoding="utf-8")
    assert "orphan zombies" in src
    assert "STATE_ZOMBIE" in src
    assert "memset(opcb, 0, PCB_STRIDE)" in src
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_3_reclamation.hxf"
    img.write_bytes(b.build_reclamation())
    r = subprocess.run(
        [str(vm), str(img)],
        capture_output=True,
        text=True,
        check=False,
        timeout=90,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0 and "guest halted" in r.stdout, blob
    for ch in "FVD":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m24.3 reclamation tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
