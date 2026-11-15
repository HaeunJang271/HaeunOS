# Getting started (HAEUN-OS 1.0)

## Requirements

- Windows 10+ (primary; Win32 viewer + Winsock backend) or POSIX host
- C17 compiler (`cc` / MinGW / MSVC via CMake — Make path is authoritative for M7–M17)
- GNU Make compatible `make`
- Python 3
- On Windows: `pwsh` for `make clean` / `build/` creation

## Build from source

```sh
make clean
make all
make test
```

`make` defaults to `all` (VM + mkfs + fsck). Release staging:

```sh
make release
```

Produces `dist/haeun-os-1.0.0/` with binaries, images, docs, and `SHA256SUMS`.

## Run (development tree)

```sh
# Hello HXF
build/haeun-vm.exe build/hello.hxf

# Firmware → kernel foundation disk
build/haeun-vm.exe --firmware build/boot.hfw --disk build/kernel_disk.raw

# Process apps launcher (needs build/app_*.hxf)
build/haeun-vm.exe build/process_apps.hxf --apps

# GUI / desktop / net (supported combinations)
build/haeun-vm.exe build/process_apps.hxf --apps --net
build/haeun-vm.exe <image.hxf> --gui --desktop --viewer --input
build/haeun-vm.exe <image.hxf> --net-real   # host UDP backend
```

Build demo images: `python tools/package_release.py --images-only`

## Run (release package)

From `dist/haeun-os-1.0.0/`:

```sh
python scripts/verify_checksums.py
bin/haeun-vm.exe images/hello.hxf
bin/haeun-vm.exe --firmware images/boot.hfw --disk images/kernel_disk.raw
# Apps: copies images/app_*.hxf → build/ then runs --apps
pwsh -File scripts/run-apps.ps1
```

## CLI summary

```text
haeun-vm <hxf>|--firmware <hfw> [--disk IMG] [--root-hash HEX]
  [--gui] [--desktop] [--apps] [--net|--net-real] [--input]
  [--viewer] [--user] [--fb-dump PPM]
```

Unknown options or missing files print an error and exit nonzero (usage → 2).

## Next documents

- [limitations.md](limitations.md) — intentional v1 caps
- [release-notes-1.0.md](release-notes-1.0.md)
- [architecture.md](architecture.md)
