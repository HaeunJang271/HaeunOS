#!/usr/bin/env python3
"""HAEUN Developer SDK CLI: new / build / verify / run."""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

SDK_ROOT = Path(__file__).resolve().parent
REPO_ROOT = SDK_ROOT.parent
# Prefer packaged python/ modules; fall back to in-tree tools/.
sys.path.insert(0, str(REPO_ROOT / "tools"))
sys.path.insert(0, str(SDK_ROOT / "python"))

import build_app_hxf as app_hxf  # noqa: E402
import haeun_toolchain as tc  # noqa: E402
from haeun_verify import verify_app_hxf  # noqa: E402


def sdk_libapp() -> Path:
    """Prefer packaged lib; fall back to repo userland."""
    packaged = SDK_ROOT / "lib" / "libapp.s"
    if packaged.is_file():
        return packaged
    return REPO_ROOT / "userland" / "libapp.s"


def find_vm() -> Path:
    """Locate haeun-vm from env or known release/build paths."""
    env = os.environ.get("HAEUN_VM")
    if env:
        p = Path(env)
        if p.is_file():
            return p
        raise SystemExit(f"HAEUN_VM not found: {env}")
    exe = "haeun-vm.exe" if sys.platform.startswith("win") else "haeun-vm"
    candidates = [
        SDK_ROOT / "bin" / exe,
        REPO_ROOT / "dist" / "haeun-os-1.0.0" / "bin" / exe,
        REPO_ROOT / "build" / exe,
    ]
    for c in candidates:
        if c.is_file():
            return c
    raise SystemExit(
        "haeun-vm not found; set HAEUN_VM or install the Release 1.0 package"
    )


def find_runner() -> Path:
    """Process image that SPAWNs every registered --app."""
    for c in (
        SDK_ROOT / "runtime" / "process_sdk.hxf",
        REPO_ROOT / "build" / "process_sdk.hxf",
    ):
        if c.is_file():
            return c
    raise SystemExit("missing process_sdk.hxf; run tools/package_sdk.py")


def cmd_new(args: argparse.Namespace) -> int:
    """Create a template application project."""
    dest = Path(args.name).resolve()
    if dest.exists():
        print(f"error: {dest} already exists", file=sys.stderr)
        return 1
    tmpl = SDK_ROOT / "templates" / "basic"
    shutil.copytree(tmpl, dest)
    app_s = dest / "src" / "app.s"
    text = app_s.read_text(encoding="utf-8").replace("APP_NAME", dest.name)
    app_s.write_text(text, encoding="utf-8")
    (dest / "README.md").write_text(
        f"# {dest.name}\n\n```\nhaeun build\nhaeun verify\nhaeun run\n```\n",
        encoding="utf-8",
    )
    print(f"created {dest}")
    return 0


def resolve_sources(project: Path) -> tuple[Path, list[Path]]:
    """Return main app.s and optional lib list from haeun.toml-like manifest."""
    manifest = project / "haeun.json"
    if manifest.is_file():
        import json

        data = json.loads(manifest.read_text(encoding="utf-8"))
        main = project / data.get("main", "src/app.s")
        libs = [project / p for p in data.get("libs", [])]
        if data.get("libapp", True):
            libs.append(sdk_libapp())
        return main, libs
    main = project / "src" / "app.s"
    if not main.is_file():
        raise SystemExit(f"missing {main}")
    return main, [sdk_libapp()]


def cmd_build(args: argparse.Namespace) -> int:
    """Build project or single .s into build/app.hxf."""
    project = Path(args.path).resolve()
    if project.is_file() and project.suffix == ".s":
        main, libs = project, [sdk_libapp()] if args.with_libapp else []
        out = Path(args.out) if args.out else project.with_suffix(".hxf")
    else:
        main, libs = resolve_sources(project)
        out = Path(args.out) if args.out else project / "build" / "app.hxf"
    out.parent.mkdir(parents=True, exist_ok=True)
    try:
        out.write_bytes(app_hxf.build_app(main, libs))
    except tc.ToolchainError as exc:
        print(f"build failed: {exc}", file=sys.stderr)
        return 1
    print(f"wrote {out} ({out.stat().st_size} bytes)")
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    """Validate an application HXF for SPAWN/APP_ENV constraints."""
    path = Path(args.hxf).resolve()
    data = path.read_bytes()
    ok, msg = verify_app_hxf(data)
    if not ok:
        print(f"verify failed: {msg}", file=sys.stderr)
        return 1
    print(f"verify ok: {msg}")
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    """Run an app HXF via process_sdk + haeun-vm --app."""
    hxf = Path(args.hxf).resolve()
    if not hxf.is_file():
        # build first if given a project dir
        if hxf.is_dir():
            rc = cmd_build(argparse.Namespace(path=str(hxf), out=None, with_libapp=True))
            if rc != 0:
                return rc
            hxf = hxf / "build" / "app.hxf"
        else:
            print(f"missing {hxf}", file=sys.stderr)
            return 1
    ok, msg = verify_app_hxf(hxf.read_bytes())
    if not ok:
        print(f"verify failed: {msg}", file=sys.stderr)
        return 1
    vm = find_vm()
    runner = find_runner()
    cmd = [str(vm), str(runner), "--app", str(hxf)]
    print("+", " ".join(cmd), flush=True)
    return subprocess.call(cmd)


def main(argv: list[str] | None = None) -> int:
    """Dispatch SDK subcommands."""
    parser = argparse.ArgumentParser(prog="haeun", description="HAEUN Developer SDK")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_new = sub.add_parser("new", help="create application project")
    p_new.add_argument("name")

    p_build = sub.add_parser("build", help="assemble app HXF")
    p_build.add_argument("path", nargs="?", default=".")
    p_build.add_argument("-o", "--out", default=None)
    p_build.add_argument("--with-libapp", action="store_true", default=True)
    p_build.add_argument("--no-libapp", action="store_false", dest="with_libapp")

    p_ver = sub.add_parser("verify", help="validate app HXF")
    p_ver.add_argument("hxf")

    p_run = sub.add_parser("run", help="run app HXF under HAEUN-OS")
    p_run.add_argument("hxf", nargs="?", default=".")

    args = parser.parse_args(argv)
    if args.cmd == "new":
        return cmd_new(args)
    if args.cmd == "build":
        return cmd_build(args)
    if args.cmd == "verify":
        return cmd_verify(args)
    if args.cmd == "run":
        return cmd_run(args)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
