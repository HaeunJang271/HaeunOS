# Release Gate Security Model

## What the authorization design prevents

The provenance-aware release path (`make release-provenanced`) is designed to prevent:

- Ordinary CLI bypasses such as `python tools/package_release.py --provenanced`
- Duck-typed fake session objects that claim smoke success without a real `GateSession`
- Fabricated HMAC authorization tokens from another run, HEAD, or session
- Reuse of a consumed (stale) authorization
- Caller-selected reduced, reordered, substituted, or expanded smoke command lists
- Unsafe `VERSION` values that could escape the intended `dist/` directory
- Fabricating build provenance by passing a command string alone (no successful
  subprocess): build entries are recorded only via `GateSession.run_and_record_build`,
  which executes `subprocess.run(..., check=True)` in the session cwd and appends
  only after exit status 0

Provenance records the official entry point, actually executed build commands with
exit statuses, and the sealed smoke command results. It does **not** invent a
Makefile target named `product-smoke`.

Legacy `make release` remains available for historical packaging and does **not**
provide smoke-authorized provenance guarantees.

## Smoke suite identity

Mandatory smoke binary stems and run argv lists share one authority in
`tools/release_paths.py` (`SMOKE_TEST_STEMS`, `mandatory_smoke_bin_relpaths`,
`mandatory_smoke_commands`). `release_gate.smoke_bin_names()` is a thin wrapper
over that list so build targets and smoke execution cannot drift.

## Same-process Python limitation

The authorization design is a **same-process integrity boundary**, not an
operating-system isolation boundary.

Hostile Python code running inside the same interpreter can still:

- Import `GateSession` and drive it with stub binaries under a temporary tree
- Monkeypatch modules, replace `subprocess.run`, or mutate private attributes
- Call internal helpers after patching repository paths

Therefore the gate prevents casual and ordinary API misuse; it does **not**
isolate a fully adversarial in-process attacker. Restricting
`run_and_record_build` does not claim defense against same-process hostile Python.

## Stronger isolation (out of scope here)

Stronger adversarial isolation would require a separate OS process for packaging,
with a protected IPC channel or another OS-enforced trust boundary (for example a
helper with distinct privileges, sealed credentials, or attestation). That is not
claimed by the current design.

## release-gate-test prerequisites

`make release-gate-test` compiles small C stubs with **GCC (`gcc` on PATH)**.
If `gcc` is missing, the suite exits with a clear error and does **not** skip
or silently PASS required checks.
