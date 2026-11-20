#!/usr/bin/env python3
"""Build release images and stage dist/haeun-os-<version>/ with checksums."""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from release_auth import (  # noqa: E402
    GateSession,
    SmokeAuthorization,
    VerifiedSmokeReport,
)
from release_paths import (  # noqa: E402
    ensure_under_directory,
    read_version_file,
    validate_version,
)
from release_verify import (  # noqa: E402
    assert_manifest_and_sums,
    sha256_file,
    write_provenance,
)

def version() -> str:
    """Read and validate the repository VERSION file."""
    return read_version_file(ROOT)

def run(cmd: list[str]) -> None:
    """Run a command; raise on failure."""
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd, cwd=str(ROOT))

def write_sums(root: Path) -> None:
    """Write SHA256SUMS for every file under root (never checksums itself)."""
    lines = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        if path.name == "SHA256SUMS":
            continue
        lines.append(f"{sha256_file(path)}  {path.relative_to(root).as_posix()}")
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
        (build / f"app_{name}.hxf").write_bytes(
            build_app_hxf.build_app(ROOT / "userland" / f"app_{name}.s")
        )
    run([sys.executable, str(ROOT / "tools" / "build_kernel_foundation.py"),
         str(build / "kernel_foundation.hxf")])
    run([sys.executable, str(ROOT / "tools" / "haeun_toolchain.py"), "as",
         str(ROOT / "firmware" / "boot.s"), str(build / "boot.hobj")])
    run([sys.executable, str(ROOT / "tools" / "haeun_toolchain.py"), "hfw",
         "-o", str(build / "boot.hfw"), str(build / "boot.hobj")])
    kh = (build / "kernel_foundation.hxf").read_bytes()
    (build / "kernel_disk.raw").write_bytes(kh + bytes((512 - (len(kh) % 512)) % 512))

def _copy_package_tree(dest: Path, build: Path, ver: str) -> None:
    """Populate a new package directory tree from build artifacts."""
    for d in ("bin", "images", "docs", "scripts"):
        (dest / d).mkdir(parents=True)
    exe = ".exe" if sys.platform.startswith("win") else ""
    for name in ("haeun-vm", "haeun-mkfs", "haeun-fsck"):
        src = build / f"{name}{exe}"
        if not src.exists():
            raise SystemExit(f"missing binary {src}; run `make all` first")
        shutil.copy2(src, dest / "bin" / src.name)
    for name in ("hello.hxf", "process_apps.hxf", "process_fault.hxf", "app_calc.hxf",
                 "app_edit.hxf", "app_files.hxf", "boot.hfw", "kernel_disk.raw"):
        shutil.copy2(build / name, dest / "images" / name)
    for doc in ("getting-started.md", "limitations.md", "release-notes-1.0.md",
                "applications.md", "boot-sequence.md"):
        shutil.copy2(ROOT / "docs" / doc, dest / "docs" / doc)
    shutil.copy2(ROOT / "LICENSE", dest / "LICENSE")
    shutil.copy2(ROOT / "VERSION", dest / "VERSION")
    readme = f"""# HAEUN-OS {ver}

See docs/getting-started.md for run commands.

Quick start (from this directory):

    bin/haeun-vm{exe} images/hello.hxf
    bin/haeun-vm{exe} images/process_apps.hxf --apps
    bin/haeun-vm{exe} --firmware images/boot.hfw --disk images/kernel_disk.raw

Verify package integrity:

    python scripts/verify_checksums.py
"""
    (dest / "README.md").write_text(readme, encoding="utf-8")
    shutil.copy2(ROOT / "tools" / "verify_checksums.py", dest / "scripts" / "verify_checksums.py")
    for script in ("run-hello.ps1", "run-apps.ps1", "run-firmware.ps1"):
        src = ROOT / "scripts" / script
        if src.exists():
            shutil.copy2(src, dest / "scripts" / script)

def stage(dist: Path, build: Path, ver: str) -> None:
    """Legacy in-place staging for `make release` (unchanged semantics)."""
    if dist.exists():
        shutil.rmtree(dist)
    _copy_package_tree(dist, build, ver)

def _git(argv: list[str]) -> str:
    """Return stripped git stdout from ROOT."""
    return subprocess.check_output(["git", *argv], cwd=str(ROOT), text=True).strip()

def _role(rel: str) -> str:
    """Return manifest role for a relative package path."""
    if rel.startswith("bin/"):
        return "binary"
    if rel.startswith("images/"):
        return "image"
    if rel.startswith("docs/"):
        return "documentation"
    if rel.startswith("scripts/"):
        return "script"
    return "metadata"

def write_manifest(root: Path) -> None:
    """Write PACKAGE_MANIFEST.json (excludes self and SHA256SUMS)."""
    files = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        if path.name in ("PACKAGE_MANIFEST.json", "SHA256SUMS"):
            continue
        rel = path.relative_to(root).as_posix()
        files.append({
            "path": rel, "size": path.stat().st_size,
            "sha256": sha256_file(path), "role": _role(rel),
        })
    (root / "PACKAGE_MANIFEST.json").write_text(
        json.dumps({"schema_version": "1", "files": files}, indent=2) + "\n",
        encoding="utf-8",
    )

def verify_checksums(root: Path) -> None:
    """Invoke tools/verify_checksums.py on root."""
    subprocess.run(
        [sys.executable, str(ROOT / "tools" / "verify_checksums.py"), str(root)],
        cwd=str(ROOT), check=True,
    )

def package_provenanced(
    *, session: GateSession, authorization: SmokeAuthorization
) -> Path:
    """Stage provenanced package after concrete GateSession authorization only."""
    if not isinstance(session, GateSession):
        raise SystemExit("refusing provenanced package: session is not GateSession")
    if not isinstance(authorization, SmokeAuthorization):
        raise SystemExit(
            "refusing provenanced package: authorization is not SmokeAuthorization"
        )
    verified = session.verify_and_consume(authorization)
    if not isinstance(verified, VerifiedSmokeReport):
        raise SystemExit(
            "refusing provenanced package: report is not VerifiedSmokeReport"
        )
    if (
        verified.smoke_status != "passed"
        or not verified.smoke_results
        or not verified.official_entry_point
        or not verified.head
        or not verified.version
    ):
        raise SystemExit("refusing provenanced package: incomplete verified report")
    ver = validate_version(version())
    if ver != verified.version:
        raise SystemExit(
            f"refusing provenanced package: VERSION mismatch "
            f"({ver} != {verified.version})"
        )
    head = _git(["rev-parse", "HEAD"])
    short = _git(["rev-parse", "--short", "HEAD"])
    if head != verified.head:
        raise SystemExit(f"HEAD mismatch before package: {head} != {verified.head}")
    if _git(["status", "--porcelain"]):
        raise SystemExit("refusing provenanced package: git is dirty")
    package_name = f"haeun-os-{ver}-{short}"
    dist_root = ROOT / "dist"
    dist_root.mkdir(parents=True, exist_ok=True)
    dist_root = ensure_under_directory(dist_root, ROOT)
    final = ensure_under_directory(dist_root / package_name, dist_root)
    staging = ensure_under_directory(
        dist_root / f".staging-{package_name}-{os.getpid()}", dist_root
    )
    if final.exists() or final.is_symlink():
        raise SystemExit(f"destination exists (refusing overwrite/delete): {final}")
    if staging.exists() or staging.is_symlink():
        if staging.is_symlink() or not staging.is_dir():
            raise SystemExit(f"refusing unsafe staging path: {staging}")
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    ensure_under_directory(staging, dist_root)
    try:
        build_images(ROOT / "build")
        _copy_package_tree(staging, ROOT / "build", ver)
        write_provenance(
            staging,
            repo_root=ROOT,
            package_name=package_name,
            ver=ver,
            smoke_status=verified.smoke_status,
            official_entry_point=verified.official_entry_point,
            build_commands=list(verified.build_commands),
            smoke_results=list(verified.smoke_results),
        )
        write_manifest(staging)
        write_sums(staging)
        verify_checksums(staging)
        assert_manifest_and_sums(staging)
        if _git(["rev-parse", "HEAD"]) != head:
            raise SystemExit("HEAD changed during packaging")
        if final.exists() or final.is_symlink():
            raise SystemExit(f"destination appeared during staging: {final}")
        staging.rename(final)
        ensure_under_directory(final, dist_root)
    except Exception:
        if staging.exists() and not staging.is_symlink():
            shutil.rmtree(staging, ignore_errors=True)
        raise
    print(f"provenanced package: {final}")
    return final

def main(argv: list[str] | None = None) -> int:
    """CLI entry for images-only or legacy release (no provenanced path)."""
    args = argv if argv is not None else sys.argv[1:]
    build = ROOT / "build"
    if "--provenanced" in args:
        print(
            "refusing: provenance-aware packaging requires `make release-provenanced` "
            "(tools/release_gate.py); CLI --provenanced is disabled",
            file=sys.stderr,
        )
        return 2
    build_images(build)
    if "--images-only" in args:
        print(f"images ready under {build}")
        return 0
    dist = ROOT / "dist" / f"haeun-os-{version()}"
    stage(dist, build, version())
    write_sums(dist)
    print(f"staged {dist}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
