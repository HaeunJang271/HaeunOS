# Environment

## Required

- C17 compiler (`cc` / GCC / compatible)
- GNU Make (or compatible; Windows: Make + PowerShell helpers in Makefile)
- Python 3 (image build scripts)
- On Windows: WinSock (`-lws2_32`), optional GDI (`-luser32 -lgdi32`) for viewer

## Platform notes

- Primary CI path in this repo historically: Windows + `make`.
- Guest ISA is interpreted; host ISA differs from guest.

## Nondeterminism

- Guest catalog runs are designed to be deterministic for metric fingerprints.
- Host `clock()` wall times vary.
- Absolute paths and timestamps in logs may differ.

## Disk

Ensure free space for `build/` (HXF images + reports). Standard campaign writes under `build/r41-standard/`.
