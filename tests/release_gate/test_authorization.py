#!/usr/bin/env python3
"""Authorization type, forgery, and lifecycle regression tests."""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

from fixtures import (
    REPO,
    expect_exit,
    fail_,
    init_git,
    pass_,
    seal_session,
    session_for,
)

sys.path.insert(0, str(REPO / "tools"))
import package_release as pr  # noqa: E402
import release_auth as ra  # noqa: E402


def test_fake_session_rejected() -> None:
    """Reject the exact Fake-session duck-typing exploit."""
    print("\n=== Fake-session rejection ===")

    class Fake:
        def verify_and_consume(self, auth):
            return type(
                "R",
                (),
                {
                    "smoke_status": "passed",
                    "smoke_results": [
                        {"command": ["x"], "exit_status": 0, "result": "passed"}
                    ],
                    "official_entry_point": "forged",
                    "build_commands": [],
                    "head": "a" * 40,
                    "version": "1.0.0",
                },
            )()

    expect_exit(
        lambda: pr.package_provenanced(session=Fake(), authorization=object()),
        "Fake-session exploit rejected",
    )


def test_forged_cross_stale_auth() -> None:
    """Reject forged MAC, cross-run, cross-HEAD, and stale authorization."""
    print("\n=== Forged / cross-run / cross-HEAD / stale auth ===")
    with tempfile.TemporaryDirectory(prefix="haeun-auth-") as td:
        repo = Path(td)
        head = init_git(repo)
        s1 = session_for(repo, "run-1")
        s1.bind_head(head)
        seal_session(repo, s1)
        auth = s1.issue_authorization()

        forged = ra.SmokeAuthorization(
            head=auth.head,
            run_id=auth.run_id,
            auth_id=auth.auth_id,
            version=auth.version,
            smoke_results_json=auth.smoke_results_json,
            mac_hex="0" * 64,
        )
        expect_exit(
            lambda: s1.verify_and_consume(forged), "forged authorization rejected"
        )

        s2 = session_for(repo, "run-2")
        s2.bind_head(head)
        seal_session(repo, s2)
        auth2 = s2.issue_authorization()
        expect_exit(
            lambda: s1.verify_and_consume(auth2),
            "cross-run authorization rejected",
        )

        s3 = session_for(repo, "run-3")
        s3.bind_head(head)
        seal_session(repo, s3)
        auth3 = s3.issue_authorization()
        wrong_head = ra.SmokeAuthorization(
            head="a" * 40,
            run_id=auth3.run_id,
            auth_id=auth3.auth_id,
            version=auth3.version,
            smoke_results_json=auth3.smoke_results_json,
            mac_hex=auth3.mac_hex,
        )
        expect_exit(
            lambda: s3.verify_and_consume(wrong_head),
            "cross-HEAD authorization rejected",
        )

        s4 = session_for(repo, "run-4")
        s4.bind_head(head)
        seal_session(repo, s4)
        auth4 = s4.issue_authorization()
        report = s4.verify_and_consume(auth4)
        if report.smoke_status != "passed":
            fail_("consume", "status")
        expect_exit(
            lambda: s4.verify_and_consume(auth4), "stale authorization rejected"
        )


def test_direct_verified_report_blocked() -> None:
    """VerifiedSmokeReport cannot be constructed without the process seal."""
    print("\n=== Direct VerifiedSmokeReport construction ===")
    try:
        ra.VerifiedSmokeReport(
            head="a" * 40,
            run_id="x",
            version="1.0.0",
            smoke_results=[],
            build_commands=[],
            official_entry_point="x",
        )
        fail_("direct VerifiedSmokeReport", "allowed")
    except TypeError:
        pass_("direct VerifiedSmokeReport construction rejected")
