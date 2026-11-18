# HAEUN-OS 1.0

HAEUN-OS is an independent operating-system research project for a custom 32-bit RISC guest (**HAEUN-32**). Guest code runs in the C17 interpreter (`src/vm.c`); the host never executes HAEUN instructions natively.

**Release 1.0** packages the M7–M17 stack: process isolation, apps launcher, GUI/net hosts, fault containment, and a reproducible Make-based build. **M19** adds a Developer SDK so external apps can be built without editing the kernel. Intentional v1 limits are listed in [docs/limitations.md](docs/limitations.md).

## Quick start

```sh
make clean
make all
make test
make release
make sdk
```

Run from the tree:

```sh
build/haeun-vm.exe build/hello.hxf
build/haeun-vm.exe --firmware build/boot.hfw --disk build/kernel_disk.raw
build/haeun-vm.exe build/process_apps.hxf --apps
build/haeun-vm.exe build/process_sdk.hxf --app path\to\app.hxf
```

Interactive 320×180 desktop (tiles → Calculator / Editor / Files):

```sh
build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer
```

- **Calculator** — digits, `+` (`=` key / keypad `+`), `-` (main or keypad), `x`, `/`, Enter (=), Backspace, Esc
- **Editor** — type letters/digits (no commands); Space, Backspace, Esc closes
- **Files** — shows registered app names only (not a full file browser); Esc closes
- Desktop: keys `1`–`3` launch apps, Esc quits the shell (click the Win32 viewer, then the app window, for keyboard focus)
- Apps open in the top band so the three tiles stay clickable; at most two apps at once (AS slot limit)

Developer SDK (`dist/haeun-sdk-1.0.0/` after `make sdk`):

```sh
python haeun.py new my-app
python haeun.py build
python haeun.py verify build\app.hxf
python haeun.py run build\app.hxf
```

See [sdk/docs/README.md](sdk/docs/README.md).

Or from the staged OS package `dist/haeun-os-1.0.0/` — see [docs/getting-started.md](docs/getting-started.md).

```text
haeun-vm <hxf>|--firmware <hfw> [--disk IMG] [--root-hash HEX]
  [--gui] [--desktop] [--apps] [--app HXF]... [--net|--net-real] [--input]
  [--viewer] [--persist] [--max-steps N] [--user] [--fb-dump PPM]
```

## Requirements

- C17 compiler, GNU Make, Python 3
- Windows: `pwsh`, WinSock (`-lws2_32`), GDI viewer (`-luser32 -lgdi32`)
- Make is the supported path for full M7–M17 tests (CMake covers an older subset)

## Release contract (supported)

| Area | Status |
|---|---|
| ISA v1–v4 + MMU/TLB | Supported |
| Firmware disk boot + HXF direct load | Supported |
| Process-v1 scheduler / syscalls | Supported |
| HFS + process FD ownership | Supported |
| GUI FB 320×180, WM, chrome, Win32 viewer | Supported |
| M22 desktop shell (`process_desktop.hxf`) | Supported (`--persist --viewer`) |
| Sockets + `--net` / `--net-real` | Supported |
| `--apps` builtins (calc / edit / files) | Supported (persistent until Esc/CLOSE) |
| M20.5 `process_launch.hxf` GUI launcher | Supported (APP_INFO/RUN/CTRL) |
| M20.6 Service Manager (ECALL 41–46) | Supported (see `docs/m20-service-manager-report.md`) |
| M20.7 File Manager (ECALL 47–50) | Supported (see `docs/m20-file-manager-report.md`) |
| `--app` + Developer SDK (`haeun` CLI) | Supported |
| M20 events / IPC / WIN_DRAW_TEXT / FILL_RECT | Supported (see `docs/m20-completion-report.md`) |
| M21 persistent runtime + input scripts | Supported |
| M15 isolation + M17 user fault → ZOMBIE | Supported |
| GUI+net mux merge | Supported |

## Documents

- [Getting started](docs/getting-started.md)
- [Developer SDK](sdk/docs/README.md)
- [Release notes 1.0](docs/release-notes-1.0.md)
- [Limitations](docs/limitations.md)
- [Architecture](docs/architecture.md) · [Boot sequence](docs/boot-sequence.md)
- [Applications](docs/applications.md) · [HXF](docs/hxf-format.md) · [Userland API](docs/userland-api.md)
- [Graphics API](docs/graphics-api.md) · [Networking](docs/networking.md) · [Filesystem](docs/filesystem.md)
- [Security](docs/security-v1.md) · [Isolation](docs/security-isolation-v1.md) · [Faults](docs/fault-handling-v1.md)
- Milestone notes: [apps](docs/apps-v1.md), [reliability](docs/reliability-v1.md), and earlier `docs/*-v1.md` series

### Research — AROS (current publication)

Host-assisted **AROS** (evaluate → recommend → safety-validate → transactional apply) is published as the R4.5/R4.6 research package on tag `v4.6.0`. Start here for external review:

| Role | Document |
|------|----------|
| **Canonical manuscript** | [research/paper/r45-main-manuscript.md](research/paper/r45-main-manuscript.md) |
| **Reproduction guide** | [research/reproduction/r45/README.md](research/reproduction/r45/README.md) |
| **Release notes (R4.6)** | [docs/r46-release-notes.md](docs/r46-release-notes.md) |
| **Publication state clarification** | [docs/r46-publication-state-clarification.md](docs/r46-publication-state-clarification.md) |
| **Review readiness** | [docs/r45-review-readiness.md](docs/r45-review-readiness.md) |

Supporting package paths (figures, tables, limitations) are listed from the reproduction README and `docs/r46-release-manifest.json`.

### Research — historical AROS drafts (R4.0–R4.3)

These remain for provenance; they are **not** the current publication manuscript:

- [R4.3 final report](docs/r43-final-report.md) (historical)
- [R4.3 active-apply audit](docs/r43-active-apply-audit.md) (historical)
- [R4.3 paper draft](research/paper/main.md) (historical draft; superseded by the R4.5 manuscript above)
- [R4.3 external reproduction notes](research/reproduction/r43-external-reproduction.md) (prior reproduction path)

## License

[MIT](LICENSE) — version [1.0.0](VERSION)
