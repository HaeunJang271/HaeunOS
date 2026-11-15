# HAEUN secure boot v2 (CLI refuse-on-mismatch)

Status: **direct `haeun-vm <image.hxf> --root-hash <64hex>` verifies the image with `haeun_sec_boot_verify` before load and refuses entry on mismatch**. Firmware/`--disk` HXF handoff still does not measure the guest kernel image inside the guest.

## CLI

```text
haeun-vm build/app.hxf --root-hash <64 hex digits>
```

- Missing or malformed hash → exit 2 (parse error).
- Digest mismatch → exit 2 (`secure boot refused HXF`).
- Without `--root-hash`, behavior is unchanged (non-secure-boot path).
- `--root-hash` with `--firmware` alone (no `--disk`) is rejected.

## API addition

`haeun_sec_boot_parse_root_hash` decodes 64 hex digits into 32 bytes.

## Tests

`sec-boot-tests` covers parse + measure/verify. CLI refusal is exercised by toolchain `test_cli_root_hash_refuses_mismatch`.

## Next

See [secure-boot-v3.md](secure-boot-v3.md) for firmware+disk root-hash gating.
