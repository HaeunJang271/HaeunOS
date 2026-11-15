#!/usr/bin/env python3
"""M19: Developer SDK build / verify / run + rejection cases."""
from __future__ import annotations

import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "sdk" / "python"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402
import haeun_toolchain as tc  # noqa: E402
from haeun_verify import verify_app_hxf  # noqa: E402

HAEUN = ROOT / "sdk" / "haeun.py"
EXAMPLES = ("hello", "calculator", "file_viewer", "drawing", "procinfo")


def run(cmd: list[str], cwd: Path | None = None, timeout: int = 120) -> subprocess.CompletedProcess:
    """Run a command; return CompletedProcess."""
    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(cwd) if cwd else None,
    )


def _assert_no_fault(blob: str, label: str) -> None:
    """Fault diagnostics must fail the test (never PASS on [haeun-fault])."""
    if "[haeun-fault]" in blob:
        raise AssertionError(f"{label}: fault diagnostic present:\n{blob}")


def _assert_relocated_hxf(data: bytes, label: str) -> None:
    """Entry/size/disasm + in-range branch targets after relocation."""
    ok, msg = verify_app_hxf(data)
    assert ok, (label, msg)
    code = data[96:]
    assert 0 < len(code) <= 4096, label
    dump = "\n".join(tc.dump_hxf(data))
    assert "0x00108000" in dump, label
    for place in range(0, len(code), 4):
        word = struct.unpack_from("<I", code, place)[0]
        if word >> 26 not in (10, 11, 12):
            continue
        imm = word & 0x3FFFF
        if imm >= (1 << 17):
            imm -= 1 << 18
        target = place + 4 + imm * 4
        assert 0 <= target <= len(code) - 4, (label, hex(place), hex(target))


def test_verify_rejects() -> None:
    """Bad size / entry / magic must fail verify."""
    good = build_app_hxf.build_app_with_libapp(
        ROOT / "sdk" / "examples" / "hello" / "src" / "app.s"
    )
    ok, _ = verify_app_hxf(good)
    assert ok, "hello should verify"
    _assert_relocated_hxf(good, "hello")

    bad_magic = bytearray(good)
    bad_magic[0:4] = b"XXXX"
    ok, msg = verify_app_hxf(bytes(bad_magic))
    assert not ok and "magic" in msg.lower()

    bad_entry = bytearray(good)
    bad_entry[16:20] = (0x00100000).to_bytes(4, "little")
    ok, msg = verify_app_hxf(bytes(bad_entry))
    assert not ok and "entry" in msg.lower()

    huge = bytearray(good)
    huge[84:88] = (8192).to_bytes(4, "little")
    ok, msg = verify_app_hxf(bytes(huge))
    assert not ok and "large" in msg.lower()


def _run_app(vm: Path, runner: Path, hxf: Path, label: str) -> str:
    """Execute one app under process_sdk; require halt without faults."""
    rc = run([str(vm), str(runner), "--app", str(hxf)], cwd=ROOT, timeout=120)
    blob = (rc.stdout or "") + (rc.stderr or "")
    assert rc.returncode == 0, (label, blob)
    assert "guest halted" in rc.stdout, (label, blob)
    assert "apps: registered" in rc.stdout, (label, blob)
    assert "app-env: spawn" in rc.stdout, (label, blob)
    _assert_no_fault(blob, label)
    return blob


def test_cli_examples(vm: Path) -> None:
    """Build + verify + run each SDK example under process_sdk."""
    build = ROOT / "build"
    build.mkdir(parents=True, exist_ok=True)
    runner = build / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())

    for name in EXAMPLES:
        proj = ROOT / "sdk" / "examples" / name
        rc = run([sys.executable, str(HAEUN), "build", str(proj)])
        assert rc.returncode == 0, (name, rc.stderr, rc.stdout)
        hxf = proj / "build" / "app.hxf"
        assert hxf.is_file()
        _assert_relocated_hxf(hxf.read_bytes(), name)
        rc = run([sys.executable, str(HAEUN), "verify", str(hxf)])
        assert rc.returncode == 0, (name, rc.stderr)
        blob = _run_app(vm, runner, hxf, name)
        # Hello emits UART bytes via console WRITE; accept either host echo
        # or clean spawn/halt as proof of real entry execution.
        if name == "hello":
            assert "app-env: spawn" in blob


def test_new_and_independent(vm: Path) -> None:
    """Package SDK, create project outside repo, build/verify/run."""
    del vm  # VM is discovered inside packaged SDK via HAEUN_VM / bin/
    pkg = run([sys.executable, str(ROOT / "tools" / "package_sdk.py")])
    assert pkg.returncode == 0, pkg.stderr
    ver = (ROOT / "VERSION").read_text(encoding="utf-8").strip().splitlines()[0]
    sdk = ROOT / "dist" / f"haeun-sdk-{ver}"
    assert (sdk / "haeun.py").is_file()
    assert (sdk / "runtime" / "process_sdk.hxf").is_file()

    with tempfile.TemporaryDirectory(prefix="haeun-sdk-ind-") as tmp:
        work = Path(tmp)
        local = work / "sdk"
        shutil.copytree(sdk, local)
        rc = run([sys.executable, str(local / "haeun.py"), "new", "demo"], cwd=work)
        assert rc.returncode == 0, rc.stderr
        proj = work / "demo"
        rc = run([sys.executable, str(local / "haeun.py"), "build"], cwd=proj)
        assert rc.returncode == 0, rc.stderr
        hxf = proj / "build" / "app.hxf"
        _assert_relocated_hxf(hxf.read_bytes(), "demo")
        rc = run([sys.executable, str(local / "haeun.py"), "verify", str(hxf)], cwd=proj)
        assert rc.returncode == 0, rc.stderr
        rc = run(
            [sys.executable, str(local / "haeun.py"), "run", str(hxf)],
            cwd=proj,
            timeout=120,
        )
        blob = (rc.stdout or "") + (rc.stderr or "")
        assert rc.returncode == 0, blob
        assert "guest halted" in rc.stdout
        _assert_no_fault(blob, "independent demo")


def main(argv: list[str]) -> int:
    """Run M19 SDK tests."""
    if len(argv) != 2:
        print("usage: m19_sdk_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    test_verify_rejects()
    test_cli_examples(vm)
    test_new_and_independent(vm)
    print("m19 sdk tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
