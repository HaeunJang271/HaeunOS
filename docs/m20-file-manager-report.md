# M20.7 File Manager — Report

## HFS audit

| Item | Finding |
|------|---------|
| Layout | Super + bitmap + inodes + data; 512 B blocks |
| Names | Basename ≤27; guest OPEN is **root-only** (no `/` paths) |
| OPEN/READ/WRITE/CLOSE | ECALL 3/2/1/4; file I/O is **1 byte** per call |
| Create | OPEN with r3≠0 creates missing root file (0644) |
| Delete / readdir / mkdir | Present in `hfs_ops.c`; **not** exposed to guest before M20.7 |
| FD table | Global 4 slots; `owner_pid`; EXIT closes by owner |
| Max file | 12 direct blocks ≈ 6 KiB |
| Apps HFS image | 32×512 B in-memory on `--apps` / `--app` |

## Classification

| Criterion | Status |
|-----------|--------|
| Root directory listing | **IMPLEMENTED** (`FS_READDIR` 47) |
| File create/read/write/close | **IMPLEMENTED** (existing 1–4) |
| File delete | **IMPLEMENTED** (`FS_UNLINK` 48) |
| File stat (size/mode) | **IMPLEMENTED** (`FS_STAT` 49) |
| Seek | **IMPLEMENTED** (`FS_SEEK` 50) |
| Nested directories / `..` | **NOT IMPLEMENTED** (flat root ABI) |
| Safe temp-file save | **NOT IMPLEMENTED** (unlink+recreate used) |
| Binary guard in GUI | **PARTIAL** (smoke does not open arbitrary binaries) |
| Interactive multi-page FM UI | **PARTIAL** (smoke GUI: list/draw + CRUD) |
| Launcher registration | **IMPLEMENTED** (`filemgr` when `app_filemgr.hxf` present) |
| Round-trip content verify | **IMPLEMENTED** (host + guest tests) |
| FD ownership | **IMPLEMENTED** |

## API (additive; 0–46 unchanged)

| # | Name | Args |
|---|------|------|
| 47 | FS_READDIR | r2=buf VA, r3=cap → r1=bytes (NUL-separated names) |
| 48 | FS_UNLINK | r2=path VA → r1=0/-1 |
| 49 | FS_STAT | r2=path → r1=0, r2=size, r3=mode |
| 50 | FS_SEEK | r2=fd, r3=offset → r1=0/-1 |

Wrappers: `userland/libfs.s`. App: `userland/app_filemgr.s` (+ `libapp.s`).

## GUI design (historical 64×32 smoke; current default FB is 320×180)

Window title “File”, draws first `FS_READDIR` entry, creates/reads/unlinks/recreates `note`, paints via `WIN_DRAW_TEXT`. Full interactive editor is out of scope for this smoke surface; CRUD paths are exercised end-to-end.

## Security

- Path copy + basename validation (reject `/` nesting, overlong names).
- SEEK/READ/WRITE/CLOSE ownership checks.
- UNLINK refuses directories (`HAEUN_HFS_ISDIR`).
- Failed ops leave prior data intact (unlink only after successful prior write verified).

## Limits

- Root flat namespace; no mkdir ECALL.
- 4 FDs global; 6 KiB/file; 16 KiB volume on apps host.
- `HAEUN_APP_MAX` raised to **5** for filemgr + prior builtins.

## Tests

| Suite | Result |
|-------|--------|
| `m20_file_tests` (round-trip, ownership, long name) | PASS |
| `m20_file_process_tests` | PASS |
| `m20_launcher_process_tests` | PASS |
| sec_dispatch / resource integrity / M14 / M20.4 | PASS |

```text
./build/m20-file-tests.exe
python tests/m20_file_process_tests.py build/haeun-vm.exe
```
