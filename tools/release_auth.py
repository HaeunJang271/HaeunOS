#!/usr/bin/env python3
"""HMAC-bound release-gate authorization (fail-closed smoke seal)."""
from __future__ import annotations

import hashlib
import hmac
import json
import re
import secrets
import subprocess
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

from release_paths import mandatory_smoke_commands, read_version_file

_VERIFIED_SEAL = object()


def _canonical_results(results: list[dict[str, Any]]) -> str:
    """Return stable JSON for HMAC binding."""
    return json.dumps(results, sort_keys=True, separators=(",", ":"))


def _norm_commands(commands: list[list[str]]) -> list[list[str]]:
    """Normalize argv lists for exact identity comparison."""
    return [list(map(str, cmd)) for cmd in commands]


@dataclass(frozen=True)
class SmokeAuthorization:
    """HMAC-bound authorization issued only by GateSession after smoke."""

    head: str
    run_id: str
    auth_id: str
    version: str
    smoke_results_json: str
    mac_hex: str


class VerifiedSmokeReport:
    """Provenance inputs produced only by GateSession.verify_and_consume."""

    __slots__ = (
        "head",
        "run_id",
        "version",
        "_smoke_results",
        "_build_commands",
        "smoke_status",
        "official_entry_point",
    )

    def __init__(
        self,
        *,
        head: str,
        run_id: str,
        version: str,
        smoke_results: list[dict[str, Any]],
        build_commands: list[dict[str, Any]],
        official_entry_point: str,
        _seal: object = None,
    ) -> None:
        """Reject direct construction without the process-local seal."""
        if _seal is not _VERIFIED_SEAL:
            raise TypeError(
                "VerifiedSmokeReport cannot be constructed directly; "
                "obtain via GateSession.verify_and_consume"
            )
        self.head = head
        self.run_id = run_id
        self.version = version
        self._smoke_results = json.loads(_canonical_results(smoke_results))
        self._build_commands = json.loads(_canonical_results(build_commands))
        self.smoke_status = "passed"
        self.official_entry_point = official_entry_point

    @property
    def smoke_results(self) -> list[dict[str, Any]]:
        """Return a defensive copy of sealed smoke results."""
        return json.loads(_canonical_results(self._smoke_results))

    @property
    def build_commands(self) -> list[dict[str, Any]]:
        """Return a defensive copy of recorded build commands."""
        return json.loads(_canonical_results(self._build_commands))


class GateSession:
    """One release-gate run: secret, run_id, smoke seal, single-use auth."""

    def __init__(
        self,
        *,
        official_entry_point: str,
        git_output: Callable[[list[str]], str],
        cwd: Path,
    ) -> None:
        """Create a session; git_output/cwd inject repo access for tests."""
        self._secret = secrets.token_bytes(32)
        self._run_id = secrets.token_hex(16)
        self._official_entry_point = official_entry_point
        self._build_commands: list[dict[str, Any]] = []
        self._git_output = git_output
        self._cwd = Path(cwd)
        self._head: str | None = None
        self._version: str | None = None
        self._results: list[dict[str, Any]] | None = None
        self._smoke_executed = False
        self._issued: set[str] = set()
        self._consumed: set[str] = set()
        self._issue_count = 0
        self._lock = threading.Lock()

    @property
    def run_id(self) -> str:
        """Return this session's run id."""
        return self._run_id

    def bind_head(self, head: str) -> None:
        """Bind the session to an exact Git HEAD SHA."""
        if not re.fullmatch(r"[0-9a-f]{40}", head):
            raise SystemExit("refusing bind_head: invalid HEAD")
        self._head = head

    def run_and_record_build(self, command: list[str]) -> None:
        """
        Run a build command with check=True and record only on success.

        Callers cannot append provenance build entries without a successful
        subprocess in this session's cwd.
        """
        if self._smoke_executed:
            raise SystemExit("refusing: cannot record build after smoke seal")
        argv = list(map(str, command))
        print("+", " ".join(argv), flush=True)
        completed = subprocess.run(argv, cwd=str(self._cwd), check=True)
        code = int(completed.returncode)
        if code != 0:
            raise SystemExit(f"refusing build record: exit_status={code}")
        self._build_commands.append({"command": argv, "exit_status": code})

    def run_checked_commands(
        self,
        commands: list[list[str]],
        *,
        root: Path,
        python_executable: str,
        make: str = "make",
    ) -> list[dict[str, Any]]:
        """Execute the mandatory smoke suite once; reject any non-canonical list."""
        if self._smoke_executed:
            raise SystemExit("refusing: smoke already sealed for this session")
        expected = mandatory_smoke_commands(
            root=root, python_executable=python_executable, make=make
        )
        if _norm_commands(commands) != _norm_commands(expected):
            raise SystemExit(
                "refusing smoke seal: command list is not the mandatory suite"
            )
        results: list[dict[str, Any]] = []
        for cmd in expected:
            print("RUN", " ".join(cmd), flush=True)
            completed = subprocess.run(cmd, cwd=str(self._cwd), check=True)
            code = int(completed.returncode)
            if code != 0:
                raise SystemExit(f"refusing smoke seal: exit_status={code}")
            results.append(
                {
                    "command": list(cmd),
                    "exit_status": code,
                    "result": "passed",
                }
            )
        if len(results) != len(expected):
            raise SystemExit("refusing smoke seal: partial smoke results")
        self._results = json.loads(_canonical_results(results))
        self._version = read_version_file(self._cwd)
        self._smoke_executed = True
        return json.loads(_canonical_results(self._results))

    def issue_authorization(self) -> SmokeAuthorization:
        """Issue a single-use HMAC authorization for this session's smoke seal."""
        with self._lock:
            if self._issue_count > 0:
                raise SystemExit("refusing issue: authorization already issued")
            if not self._smoke_executed or self._results is None or self._head is None:
                raise SystemExit("refusing issue: smoke not executed in this session")
            if self._version is None:
                raise SystemExit("refusing issue: VERSION not sealed")
            current = self._git_output(["rev-parse", "HEAD"]).strip()
            if current != self._head:
                raise SystemExit(
                    f"refusing issue: HEAD changed ({self._head} -> {current})"
                )
            if self._git_output(["status", "--porcelain"]).strip():
                raise SystemExit("refusing issue: git is dirty")
            live_ver = read_version_file(self._cwd)
            if live_ver != self._version:
                raise SystemExit("refusing issue: VERSION changed since smoke seal")
            auth_id = secrets.token_hex(16)
            payload = self._payload(
                self._head,
                self._run_id,
                auth_id,
                self._version,
                self._results,
                self._build_commands,
                self._official_entry_point,
            )
            mac_hex = hmac.new(self._secret, payload, hashlib.sha256).hexdigest()
            auth = SmokeAuthorization(
                head=self._head,
                run_id=self._run_id,
                auth_id=auth_id,
                version=self._version,
                smoke_results_json=_canonical_results(self._results),
                mac_hex=mac_hex,
            )
            self._issued.add(auth_id)
            self._issue_count += 1
            return auth

    def verify_and_consume(self, auth: SmokeAuthorization) -> VerifiedSmokeReport:
        """Verify authorization against this session and consume it atomically."""
        with self._lock:
            if not isinstance(auth, SmokeAuthorization):
                raise SystemExit("refusing: invalid authorization type")
            if auth.auth_id in self._consumed:
                raise SystemExit("refusing: authorization already consumed (stale)")
            if auth.auth_id not in self._issued:
                raise SystemExit("refusing: authorization not issued by this session")
            if auth.run_id != self._run_id:
                raise SystemExit("refusing: authorization from a different gate run")
            if self._head is None or auth.head != self._head:
                raise SystemExit("refusing: authorization HEAD mismatch (session)")
            current = self._git_output(["rev-parse", "HEAD"]).strip()
            if current != auth.head:
                raise SystemExit(
                    f"refusing: authorization HEAD mismatch (repo {current})"
                )
            if self._git_output(["status", "--porcelain"]).strip():
                raise SystemExit("refusing: git is dirty at package time")
            live_ver = read_version_file(self._cwd)
            if live_ver != auth.version:
                raise SystemExit("refusing: VERSION mismatch at consume")
            results = json.loads(auth.smoke_results_json)
            payload = self._payload(
                auth.head,
                auth.run_id,
                auth.auth_id,
                auth.version,
                results,
                self._build_commands,
                self._official_entry_point,
            )
            expected = hmac.new(self._secret, payload, hashlib.sha256).hexdigest()
            if not hmac.compare_digest(expected, auth.mac_hex):
                raise SystemExit(
                    "refusing: authorization MAC invalid (forged/corrupt)"
                )
            if self._results is not None and _canonical_results(self._results) != (
                auth.smoke_results_json
            ):
                raise SystemExit("refusing: authorization results diverge from seal")
            self._consumed.add(auth.auth_id)
            return VerifiedSmokeReport(
                head=auth.head,
                run_id=auth.run_id,
                version=auth.version,
                smoke_results=results,
                build_commands=list(self._build_commands),
                official_entry_point=self._official_entry_point,
                _seal=_VERIFIED_SEAL,
            )

    @staticmethod
    def _payload(
        head: str,
        run_id: str,
        auth_id: str,
        version: str,
        results: list[dict[str, Any]],
        build_commands: list[dict[str, Any]],
        official_entry_point: str,
    ) -> bytes:
        """Build HMAC payload bytes including VERSION and execution identity."""
        body = {
            "head": head,
            "run_id": run_id,
            "auth_id": auth_id,
            "version": version,
            "package_identity": f"haeun-os-{version}",
            "official_entry_point": official_entry_point,
            "build_commands": build_commands,
            "smoke_results": results,
        }
        return json.dumps(body, sort_keys=True, separators=(",", ":")).encode("utf-8")
