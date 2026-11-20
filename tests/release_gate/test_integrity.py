#!/usr/bin/env python3
"""Concurrency, checksum, and temporary package integrity regressions."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import threading
from pathlib import Path
from unittest import mock

from fixtures import (
    REPO,
    expect_exit,
    fail_,
    init_git,
    install_build_recorder,
    pass_,
    seal_session,
    session_for,
)

sys.path.insert(0, str(REPO / "tools"))
import package_release as pr  # noqa: E402
import release_auth as ra  # noqa: E402
import release_verify as rv  # noqa: E402


def test_concurrent_double_consumption() -> None:
    """Exactly one of two concurrent consumers may succeed."""
    print("\n=== Concurrent double consumption ===")
    with tempfile.TemporaryDirectory(prefix="haeun-conc-") as td:
        repo = Path(td)
        head = init_git(repo)
        s = session_for(repo, "conc")
        s.bind_head(head)
        seal_session(repo, s)
        auth = s.issue_authorization()
        outcomes: list[str] = []
        barrier = threading.Barrier(2)

        def worker() -> None:
            barrier.wait()
            try:
                s.verify_and_consume(auth)
                outcomes.append("ok")
            except SystemExit:
                outcomes.append("fail")

        t1 = threading.Thread(target=worker)
        t2 = threading.Thread(target=worker)
        t1.start()
        t2.start()
        t1.join()
        t2.join()
        if outcomes.count("ok") != 1 or outcomes.count("fail") != 1:
            fail_("concurrent consume", str(outcomes))
        pass_("exactly one concurrent consumer succeeded")


def test_manifest_checksum_tampering() -> None:
    """Independent checksum verification detects tampering."""
    print("\n=== Manifest/checksum tampering ===")
    with tempfile.TemporaryDirectory(prefix="haeun-sum-") as td:
        root = Path(td)
        (root / "bin").mkdir()
        (root / "bin" / "a.bin").write_bytes(b"abc")
        (root / "BUILD_PROVENANCE.json").write_text(
            json.dumps({"schema_version": "1"}) + "\n", encoding="utf-8"
        )
        files = []
        for path in sorted(p for p in root.rglob("*") if p.is_file()):
            if path.name in ("PACKAGE_MANIFEST.json", "SHA256SUMS"):
                continue
            rel = path.relative_to(root).as_posix()
            files.append(
                {
                    "path": rel,
                    "size": path.stat().st_size,
                    "sha256": rv.sha256_file(path),
                    "role": "metadata",
                }
            )
        (root / "PACKAGE_MANIFEST.json").write_text(
            json.dumps({"schema_version": "1", "files": files}, indent=2) + "\n",
            encoding="utf-8",
        )
        lines = []
        for path in sorted(p for p in root.rglob("*") if p.is_file()):
            if path.name == "SHA256SUMS":
                continue
            lines.append(
                f"{rv.sha256_file(path)}  {path.relative_to(root).as_posix()}"
            )
        (root / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")
        rv.assert_manifest_and_sums(root)
        pass_("checksum+manifest independent verification")
        (root / "bin" / "a.bin").write_bytes(b"TAMPER")
        expect_exit(
            lambda: rv.assert_manifest_and_sums(root),
            "manifest/checksum tampering detected",
        )


def test_build_provenance_requires_real_subprocess() -> None:
    """Build provenance records only after a successful subprocess, not by string."""
    print("\n=== Build provenance real-subprocess API ===")
    if hasattr(ra.GateSession, "record_build_command"):
        fail_(
            "record_build_command API",
            "public string-recording API must not exist",
        )
    pass_("no record_build_command string API")

    with tempfile.TemporaryDirectory(prefix="haeun-bprov-") as td:
        repo = Path(td)
        head = init_git(repo)
        s = session_for(repo, "build-api")
        s.bind_head(head)
        make_s = install_build_recorder(repo)
        try:
            s.run_and_record_build([make_s, "nope"])
            fail_("failed build record", "did not raise")
        except (SystemExit, subprocess.CalledProcessError, OSError):
            pass_("failed subprocess does not record build")
        if s._build_commands:
            fail_("failed build leaked into record", str(s._build_commands))
        pass_("failed command left build_commands empty")

        s.run_and_record_build([make_s, "clean"])
        s.run_and_record_build([make_s, "all"])
        recorded = list(s._build_commands)
        expected = [
            {"command": [make_s, "clean"], "exit_status": 0},
            {"command": [make_s, "all"], "exit_status": 0},
        ]
        if recorded != expected:
            fail_("successful build record", str(recorded))
        pass_("successful subprocesses recorded with exit_status")


def test_temporary_verified_package() -> None:
    """Authorized packaging writes only under an isolated temp dist."""
    print("\n=== Temporary verified package ===")
    with tempfile.TemporaryDirectory(prefix="haeun-pkg-") as td:
        repo = Path(td)
        head = init_git(repo)
        s = session_for(repo, "make release-provenanced")
        s.bind_head(head)
        make_s = install_build_recorder(repo)
        s.run_and_record_build([make_s, "clean"])
        s.run_and_record_build([make_s, "all"])
        expected_builds = [
            {"command": [make_s, "clean"], "exit_status": 0},
            {"command": [make_s, "all"], "exit_status": 0},
        ]
        results, _, _ = seal_session(repo, s)
        auth = s.issue_authorization()

        def fake_build(build: Path) -> None:
            build.mkdir(parents=True, exist_ok=True)

        def fake_copy(dest: Path, build: Path, ver: str) -> None:
            dest.mkdir(parents=True, exist_ok=True)
            (dest / "LICENSE").write_text("x\n", encoding="utf-8")

        with mock.patch.object(pr, "ROOT", repo), mock.patch.object(
            pr, "build_images", fake_build
        ), mock.patch.object(pr, "_copy_package_tree", fake_copy), mock.patch.object(
            pr, "verify_checksums", lambda root: None
        ):
            dest = pr.package_provenanced(session=s, authorization=auth)
        if REPO.as_posix() in dest.resolve().as_posix() and "/dist/" in dest.as_posix():
            # Allow only if somehow nested; project dist must not be used.
            if str(REPO / "dist") in str(dest.resolve()):
                fail_("isolation", str(dest))
        if str((REPO / "dist").resolve()) in str(dest.resolve()):
            fail_("wrote into project dist", str(dest))
        prov = json.loads((dest / "BUILD_PROVENANCE.json").read_text(encoding="utf-8"))
        if prov["git_commit"] != head:
            fail_("pkg head", prov["git_commit"])
        if prov["official_entry_point"] != "make release-provenanced":
            fail_("entry point", prov["official_entry_point"])
        if "product-smoke" in json.dumps(prov):
            fail_("provenance", "records product-smoke label")
        if prov.get("build_commands") != expected_builds:
            fail_("build_commands", str(prov.get("build_commands")))
        if prov["smoke_commands"] != results:
            fail_("smoke_commands", "mismatch")
        for item in prov["smoke_commands"]:
            if "exit_status" not in item:
                fail_("smoke exit_status", str(item))
        rv.assert_manifest_and_sums(dest)
        pass_("temporary package with verified authorization")
        pass_("provenance fields record actual executed commands")
