#!/usr/bin/env python3
"""M20.3: SDK text_demo / text_layout / text_terminal under process_sdk."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_sdk  # noqa: E402

HAEUN = ROOT / "sdk" / "haeun.py"
EXAMPLES = ("text_demo", "text_layout", "text_terminal")


def main(argv: list[str]) -> int:
    """Build and run the three M20.3 text SDK examples."""
    if len(argv) != 2:
        print("usage: m20_text_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    for name in EXAMPLES:
        proj = ROOT / "sdk" / "examples" / name
        r = subprocess.run(
            [sys.executable, str(HAEUN), "build", str(proj)],
            capture_output=True,
            text=True,
            check=False,
            cwd=str(ROOT),
        )
        assert r.returncode == 0, (name, r.stderr, r.stdout)
        hxf = proj / "build" / "app.hxf"
        cmd = [str(vm), str(runner), "--app", str(hxf)]
        if name == "text_terminal":
            cmd.extend(["--gui", "--desktop", "--input"])
        else:
            cmd.extend(["--gui", "--desktop"])
        rr = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False,
            timeout=180,
            cwd=str(ROOT),
        )
        blob = (rr.stdout or "") + (rr.stderr or "")
        assert rr.returncode == 0, (name, blob)
        assert "guest halted" in rr.stdout, (name, blob)
        assert "[haeun-fault]" not in blob, (name, blob)
    print("m20 text process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
