# HAEUN secure boot v1

Status: **host-side SHA-256 measurement and root-hash / allow-list verification of HXF images are implemented and tested**. Guest firmware does not yet refuse handoff on mismatch; the direct `haeun-vm <image.hxf>` path remains a non-secure-boot loader unless a future CLI flag wires this API.

## Milestone scope

| Component | Status |
|---|---|
| SHA-256 (`haeun_sha256`) | Done |
| `haeun_sec_boot_measure` after `validate_exec` | Done |
| Single root-hash verify | Done |
| Allow-list verify | Done |
| Logged denials | Done |
| Firmware/HFW signature field | Not started |
| Guest firmware refuse-on-mismatch | Not started |

## Rules

1. Measurement runs only after `haeun_sec_validate_exec` accepts the image.
2. Digest is SHA-256 over the full HXF file bytes (header + segments).
3. Mismatch or empty allow-list returns `HAEUN_SEC_DENIED` and logs.

## Tests

`tests/sec_boot_tests.c`: NIST empty/`abc` digests; HXF measure/verify/allow-list.

## Next

See [secure-boot-v2.md](secure-boot-v2.md) for CLI `--root-hash`. Firmware refuse-on-mismatch remains future work.
