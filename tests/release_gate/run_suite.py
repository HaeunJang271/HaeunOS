#!/usr/bin/env python3
"""Run persistent release-gate authorization regression suite."""
from __future__ import annotations

import sys
import traceback
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from fixtures import require_gcc

import test_authorization
import test_integrity
import test_smoke


def main() -> int:
    """Execute all release-gate regression modules; return process status."""
    # Fail closed before any PASS lines if the C compiler is missing.
    require_gcc()
    try:
        test_authorization.test_fake_session_rejected()
        test_authorization.test_forged_cross_stale_auth()
        test_authorization.test_direct_verified_report_blocked()
        test_smoke.test_smoke_build_and_run_lists_match()
        test_smoke.test_smoke_command_list_enforcement()
        test_smoke.test_version_and_path_jail()
        test_integrity.test_concurrent_double_consumption()
        test_integrity.test_manifest_checksum_tampering()
        test_integrity.test_build_provenance_requires_real_subprocess()
        test_integrity.test_temporary_verified_package()
        print("\nALL_RELEASE_GATE_REGRESSION_TESTS_PASSED", flush=True)
        return 0
    except SystemExit as exc:
        return int(exc.code) if isinstance(exc.code, int) else 1
    except Exception:
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
