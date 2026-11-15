# SDK Installation

## Requirements

- Python 3.10+
- HAEUN-OS Release 1.0 package **or** an in-tree `build/haeun-vm` binary
- Windows, macOS, or Linux

## From `dist/haeun-sdk-<version>/`

1. Build the VM once in the OS tree: `make`
2. Stage the SDK: `make sdk` (runs `tools/package_sdk.py`)
3. Open a shell in `dist/haeun-sdk-<version>/`
4. Confirm tools:

```text
python haeun.py --help
```

On Windows you may use `haeun.cmd`. Set `HAEUN_VM` if the packaged `bin/haeun-vm` is moved.

## Layout

| Path | Role |
|------|------|
| `haeun.py` / `haeun.cmd` | CLI |
| `bin/haeun-vm*` | Guest VM |
| `runtime/process_sdk.hxf` | SPAWN runner image |
| `lib/libapp.s` | Standard library |
| `python/` | Assembler + HXF helpers |
| `templates/basic/` | `haeun new` template |
| `examples/` | Reference apps |
| `docs/` | This documentation |
