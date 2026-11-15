#!/usr/bin/env python3
"""Build release images and stage dist/haeun-os-<version>/ with checksums."""
from __future__ import annotations

import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))


def version() -> str:
    """Read VERSION file (first non-empty line)."""
    text = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    line = text.splitlines()[0].strip() if text else "0.0.0"
    return line


def run(cmd: list[str]) -> None:
    """Run a command; raise on failure."""
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd, cwd=str(ROOT))


def sha256_file(path: Path) -> str:
    """Return hex SHA-256 of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def write_sums(root: Path) -> None:
    """Write SHA256SUMS for every regular file under root (relative paths)."""
    lines: list[str] = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        if path.name == "SHA256SUMS":
            continue
        rel = path.relative_to(root).as_posix()
        lines.append(f"{sha256_file(path)}  {rel}")
    (root / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")


def build_images(build: Path) -> None:
    """Assemble demo / apps / firmware artifacts into build/."""
    import build_app_hxf
    import build_process_apps
    import build_process_fault
    import haeun_toolchain as tc

    build.mkdir(parents=True, exist_ok=True)
    hello = tc.assemble((ROOT / "examples" / "hello.s").read_text(encoding="utf-8"))
    (build / "hello.hxf").write_bytes(tc.link_hobjs([hello]))
    (build / "process_apps.hxf").write_bytes(build_process_apps.build())
    (build / "process_fault.hxf").write_bytes(build_process_fault.build())
    for name in ("calc", "edit", "files"):
        src = ROOT / "userland" / f"app_{name}.s"
        out = build / f"app_{name}.hxf"
        out.write_bytes(build_app_hxf.build_app(src))
    # Firmware + padded kernel disk for --firmware path.
    run([sys.executable, str(ROOT / "tools" / "build_kernel_foundation.py"),
         str(build / "kernel_foundation.hxf")])
    run([sys.executable, str(ROOT / "tools" / "haeun_toolchain.py"), "as",
         str(ROOT / "firmware" / "boot.s"), str(build / "boot.hobj")])
    run([sys.executable, str(ROOT / "tools" / "haeun_toolchain.py"), "hfw",
         "-o", str(build / "boot.hfw"), str(build / "boot.hobj")])
    kh = (build / "kernel_foundation.hxf").read_bytes()
    pad = (512 - (len(kh) % 512)) % 512
    (build / "kernel_disk.raw").write_bytes(kh + bytes(pad))


def stage(dist: Path, build: Path, ver: str) -> None:
    """Copy runtime files into a clean dist tree."""
    if dist.exists():
        shutil.rmtree(dist)
    bin_dir = dist / "bin"
    img_dir = dist / "images"
    doc_dir = dist / "docs"
    script_dir = dist / "scripts"
    for d in (bin_dir, img_dir, doc_dir, script_dir):
        d.mkdir(parents=True)
    exe = ".exe" if sys.platform.startswith("win") else ""
    for name in ("haeun-vm", "haeun-mkfs", "haeun-fsck"):
        src = build / f"{name}{exe}"
        if not src.exists():
            raise SystemExit(f"missing binary {src}; run `make all` first")
        shutil.copy2(src, bin_dir / src.name)
    for name in (
        "hello.hxf",
        "process_apps.hxf",
        "process_fault.hxf",
        "app_calc.hxf",
        "app_edit.hxf",
        "app_files.hxf",
        "boot.hfw",
        "kernel_disk.raw",
    ):
        shutil.copy2(build / name, img_dir / name)
    for doc in (
        "getting-started.md",
        "limitations.md",
        "release-notes-1.0.md",
        "applications.md",
        "boot-sequence.md",
    ):
        shutil.copy2(ROOT / "docs" / doc, doc_dir / doc)
    shutil.copy2(ROOT / "LICENSE", dist / "LICENSE")
    shutil.copy2(ROOT / "VERSION", dist / "VERSION")
    readme = f"""# HAEUN-OS {ver}

See docs/getting-started.md for run commands.

Quick start (from this directory):

    bin/haeun-vm{exe} images/hello.hxf
    bin/haeun-vm{exe} images/process_apps.hxf --apps
    bin/haeun-vm{exe} --firmware images/boot.hfw --disk images/kernel_disk.raw

Verify package integrity:

    python scripts/verify_checksums.py
"""
    (dist / "README.md").write_text(readme, encoding="utf-8")
    shutil.copy2(ROOT / "tools" / "verify_checksums.py", script_dir / "verify_checksums.py")
    for script in ("run-hello.ps1", "run-apps.ps1", "run-firmware.ps1"):
        src = ROOT / "scripts" / script
        if src.exists():
            shutil.copy2(src, script_dir / script)


def main(argv: list[str] | None = None) -> int:
    """Build images, stage dist/haeun-os-<ver>, write SHA256SUMS."""
    args = argv if argv is not None else sys.argv[1:]
    ver = version()
    build = ROOT / "build"
    build_images(build)
    if "--images-only" in args:
        print(f"images ready under {build}")
        return 0
    dist = ROOT / "dist" / f"haeun-os-{ver}"
    stage(dist, build, ver)
    write_sums(dist)
    print(f"staged {dist}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
