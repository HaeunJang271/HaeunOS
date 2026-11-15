# HAEUN secure boot v3 (firmware disk HXF gate)

Status: **`haeun-vm --firmware … --disk … --root-hash <64hex>` verifies the HXF at LBA 0 before staging firmware / running the guest**. Mismatch refuses the boot (exit 2). Guest `firmware/boot.s` still does not compute SHA-256 itself; the host enforces the policy at the disk-boot boundary.

## Behavior

1. Parse `--root-hash`.
2. Read `file_size` from the disk HXF header (offset 28).
3. `haeun_sec_boot_verify` on those bytes.
4. Only then load HFW and run.

Without `--root-hash`, firmware disk boot is unchanged (non-secure-boot path).

## API

`haeun_sec_boot_verify_disk_hxf(disk, disk_size, expected)`

## Tests

- `sec-boot-tests`: disk verify accept/deny
- toolchain: `test_firmware_disk_root_hash_refuses_mismatch`

## Not claimed

- In-guest SHA-256 inside `boot.s`
- Persistent root hash in Boot ROM MMIO
