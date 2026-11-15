#!/usr/bin/env python3
"""Stage dist/haeun-sdk-<version>/ for external developers."""
from __future__ import annotations

import hashlib
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def version() -> str:
    """Read VERSION file (first non-empty line)."""
    text = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    return text.splitlines()[0].strip() if text else "0.0.0"


def sha256_file(path: Path) -> str:
    """Return hex SHA-256 of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def write_sums(root: Path) -> None:
    """Write SHA256SUMS for files under root."""
    lines: list[str] = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        if path.name == "SHA256SUMS":
            continue
        rel = path.relative_to(root).as_posix()
        lines.append(f"{sha256_file(path)}  {rel}")
    (root / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")


def ensure_vm(build: Path) -> Path:
    """Return haeun-vm path; require prior `make` / `make all`."""
    exe = "haeun-vm.exe" if sys.platform.startswith("win") else "haeun-vm"
    for c in (build / exe, ROOT / "dist" / f"haeun-os-{version()}" / "bin" / exe):
        if c.is_file():
            return c
    raise SystemExit(f"missing {exe}; run `make` first")


def main() -> int:
    """Build SDK runtime artifacts and stage dist package."""
    sys.path.insert(0, str(ROOT / "tools"))
    import build_process_sdk  # noqa: E402

    ver = version()
    build = ROOT / "build"
    build.mkdir(parents=True, exist_ok=True)
    sdk_src = ROOT / "sdk"
    dist = ROOT / "dist" / f"haeun-sdk-{ver}"
    if dist.exists():
        shutil.rmtree(dist)

    process_sdk = build / "process_sdk.hxf"
    process_sdk.write_bytes(build_process_sdk.build())

    vm = ensure_vm(build)
    bin_dir = dist / "bin"
    py_dir = dist / "python"
    for d in (
        bin_dir,
        py_dir,
        dist / "lib",
        dist / "runtime",
        dist / "templates",
        dist / "examples",
        dist / "docs",
    ):
        d.mkdir(parents=True, exist_ok=True)

    shutil.copy2(vm, bin_dir / vm.name)
    shutil.copy2(process_sdk, dist / "runtime" / "process_sdk.hxf")
    shutil.copy2(ROOT / "userland" / "libapp.s", dist / "lib" / "libapp.s")
    shutil.copy2(sdk_src / "haeun.py", dist / "haeun.py")
    shutil.copy2(sdk_src / "python" / "haeun_verify.py", py_dir / "haeun_verify.py")
    for name in ("haeun_toolchain.py", "haeun_isa.py", "build_app_hxf.py"):
        shutil.copy2(ROOT / "tools" / name, py_dir / name)
    isa_dst = py_dir / "isa"
    if isa_dst.exists():
        shutil.rmtree(isa_dst)
    shutil.copytree(ROOT / "tools" / "isa", isa_dst)

    # In-package layout: haeun.py treats parent as REPO_ROOT — mirror sdk/ layout.
    # Override: copy tree so SDK_ROOT==dist and tools live in python/.
    shutil.copytree(sdk_src / "templates", dist / "templates", dirs_exist_ok=True)
    shutil.copytree(sdk_src / "examples", dist / "examples", dirs_exist_ok=True)
    if (sdk_src / "docs").is_dir():
        shutil.copytree(sdk_src / "docs", dist / "docs", dirs_exist_ok=True)

    # Windows + POSIX launchers
    (dist / "haeun.cmd").write_text(
        "@echo off\r\npython \"%~dp0haeun.py\" %*\r\n",
        encoding="utf-8",
    )
    (dist / "haeun").write_text(
        "#!/usr/bin/env bash\nexec python3 \"$(dirname \"$0\")/haeun.py\" \"$@\"\n",
        encoding="utf-8",
    )
    (dist / "README.md").write_text(
        f"# HAEUN Developer SDK {ver}\n\n"
        "See `docs/` for installation and API reference.\n\n"
        "```\n"
        "python haeun.py new my-app\n"
        "cd my-app\n"
        "python ..\\haeun.py build\n"
        "python ..\\haeun.py verify build\\app.hxf\n"
        "python ..\\haeun.py run build\\app.hxf\n"
        "```\n"
        "Or add this directory to PATH and use `haeun.cmd` / `./haeun`.\n",
        encoding="utf-8",
    )

    write_sums(dist)
    print(f"staged {dist}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
