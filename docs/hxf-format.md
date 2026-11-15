# HXF format (Release 1.0)

Executable container **HXF1** used by `haeun-vm` and the toolchain linker.

Authoritative field layout and validation rules: toolchain + `src/hxf.c` / [hobj-v1.md](hobj-v1.md) evolution notes.

## Essentials verified in this tree

- Magic `HXF1`
- ISA version 1–4
- Segment table; process apps require user segment VA `0x00108000`
- Host rejects truncated/invalid images before mutating RAM
- App path also runs `haeun_sec_validate_exec`

## Build

```sh
python tools/haeun_toolchain.py as examples/hello.s build/hello.hobj
python tools/haeun_toolchain.py ld -o build/hello.hxf build/hello.hobj
```

Firmware wraps HOBJ as HFW: `haeun_toolchain.py hfw -o build/boot.hfw build/boot.hobj`.
