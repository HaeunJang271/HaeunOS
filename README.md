# HAEUN-OS 1.0

[![Linux CI](https://github.com/HaeunJang271/HaeunOS/actions/workflows/linux-ci.yml/badge.svg?branch=main)](https://github.com/HaeunJang271/HaeunOS/actions/workflows/linux-ci.yml)

### Project Overview

HAEUN-OS is an independent operating-system research project built around a custom 32-bit guest ISA (**HAEUN-32**) and a host-side C17 virtual machine. Guest instructions execute through the interpreter in `src/vm.c`; they are not executed natively by the host CPU. HAEUN-OS is not presented as a native PC-bootable operating system.

HAEUN-OS is not affiliated with, derived from, or related to Amiga AROS (AROS Research Operating System). The acronym overlap is coincidental.

### Why This Project Exists

The project provides a controlled experimental environment for studying operating-system mechanisms where kernel behavior, process management, isolation, and runtime policy changes can be observed and tested on a research guest. A related research track investigates host-assisted evaluation of scheduling policies and safety-controlled mutation of a narrow guest policy word. Intentional v1 limits are listed in [docs/limitations.md](docs/limitations.md).

### What Actually Runs

Release 1.0 is a Make-based stack (`make all`, `make test`, `make release`, `make sdk`) that builds the host VM and guest images under `build/`. Features below are present in the current sources; corresponding unit/process tests exist in the Make `test` graph and/or `tests/` (this README does not assert a fresh green `make test` run). Caps and host constraints are in [docs/limitations.md](docs/limitations.md); the supported matrix is in [Release contract](#release-contract-supported).

- **HAEUN-32 ISA and MMU/TLB** — ISA v1–v4 interpretation with MMU/TLB paths in the host VM (`src/vm.c`, `src/trap.c`, MMU/TLB modules; Make builds/runs `mmu-tests` and `tlb-tests`).
- **Firmware boot and HXF load** — Firmware disk boot (`--firmware` / `--disk`) and direct load of HXF guest programs (HAEUN executable format) via `build/haeun-vm` (Windows: `haeun-vm.exe`).
- **Processes, scheduling, syscalls** — Process-v1 guest kernel with cooperative yield and timer preemption; guest ECALL (system-call) surface used by process images.
- **Isolation and faults** — Address-space isolation and user-fault handling (including ZOMBIE paths); see `tests/m15_isolation_tests.c` and `tests/m17_fault_tests.c` / related process tests.
- **Filesystem and FDs** — HFS with process file-descriptor ownership (`proc-hfs-tests` and related HFS tests).
- **Framebuffer, window manager, viewer** — 320×180 framebuffer, window manager/chrome; interactive Win32 GDI viewer (`--viewer`). Non-Windows hosts have no supported interactive viewer (stub).
- **Desktop shell and builtins** — `process_desktop.hxf` shell and Calculator / Editor / Files under `--apps` (scripted guest coverage; not physical HID validation).
- **Networking** — Socket ECALLs with `--net` / `--net-real`; external DNS remains opt-in (`HAEUN_NET_EXTERNAL=1`).
- **Developer SDK** — `make sdk` produces `dist/haeun-sdk-1.0.0/` and the `haeun` CLI for external guest apps without editing the kernel (`tests/m19_sdk_tests.py`).

### AROS Research Framework

**AROS** here is a host-assisted research framework for runtime scheduling-policy evaluation—not the guest OS name and not Amiga AROS. Host tooling scores and may recommend RR (`0`) or preemptive FIFO (`1`); SHADOW is the default (evaluate/report without writing policy). APPLY is explicit opt-in.

Policy mutation targets the guest policy word at `0xA474` through a host-side request → prepare/validate → commit path (`haeun_recfg_*` / `haeun_opt_apply`). Recommendation alone does not authorize the write. The between-`haeun_vm_step()` safe point is a single-threaded design assumption for serialized commit, not a runtime gate inside `commit()`. On failure after write, rollback restores the previous policy ID only—not full VM/PCB/memory state.

AROS does not claim autonomous always-on policy selection, general scheduling-performance superiority, ACID or full-system rollback, SMP safety, hardware atomicity, or third-party reproduction. Frozen publication: tag `v4.6.0` (R4.5/R4.6). Post-tag claim reframing: R4.8 on `main` (does not replace `v4.6.0`). See [Research — AROS (frozen canonical release)](#research-aros-frozen-canonical-release) below.

## Quick start

Shortest reliable sample path: `make all`, then run the hello HXF with the host VM binary (`build/haeun-vm.exe` on Windows, `build/haeun-vm` on POSIX).

```sh
make clean
make all
make test
make release
make sdk
```

Run from the tree (Windows examples; drop `.exe` on POSIX):

```sh
build/haeun-vm.exe build/hello.hxf
build/haeun-vm.exe --firmware build/boot.hfw --disk build/kernel_disk.raw
build/haeun-vm.exe build/process_apps.hxf --apps
build/haeun-vm.exe build/process_sdk.hxf --app path\to\app.hxf
```

Interactive 320×180 desktop (tiles → Calculator / Editor / Files; Win32 viewer):

```sh
build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer
```

- **Calculator** — digits, `+` (`=` key / keypad `+`), `-` (main or keypad), `x`, `/`, Enter (=), Backspace, Esc
- **Editor** — type letters/digits (no commands); Space, Backspace, Esc closes
- **Files** — shows registered app names only (not a full file browser); Esc closes
- Desktop: keys `1`–`3` launch apps, Esc quits the shell (click the Win32 viewer, then the app window, for keyboard focus)
- Apps open in the top band so the three tiles stay clickable; at most two apps at once (AS slot limit)

Developer SDK (`dist/haeun-sdk-1.0.0/` after `make sdk`; run these from that directory):

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

### Research — AROS (frozen canonical release)

Host-assisted **AROS** (evaluate → recommend → safety-validate → scoped policy-word apply) is published as the R4.5/R4.6 research package on tag `v4.6.0`. That tagged package remains the **immutable historical release** for external review of the frozen publication.

| Role | Document |
|------|----------|
| **Frozen canonical manuscript** | [research/paper/r45-main-manuscript.md](research/paper/r45-main-manuscript.md) |
| **Reproduction guide** | [research/reproduction/r45/README.md](research/reproduction/r45/README.md) |
| **Release notes (R4.6)** | [docs/r46-release-notes.md](docs/r46-release-notes.md) |
| **Publication state clarification** | [docs/r46-publication-state-clarification.md](docs/r46-publication-state-clarification.md) |
| **Review readiness** | [docs/r45-review-readiness.md](docs/r45-review-readiness.md) |

Supporting package paths (figures, tables, limitations) are listed from the reproduction README and `docs/r46-release-manifest.json`.

### Research — AROS revised manuscript (post-v4.6.0)

R4.8 revises claim framing and incorporates the R47 post-publication recommendation-discrimination follow-up. These paths are present on `main` as a **post-`v4.6.0` revision**. They do **not** replace tag `v4.6.0` or the frozen R4.5/R4.6 package.

| Role | Document |
|------|----------|
| **Revised manuscript (R4.8)** | [research/paper/r48-main-manuscript.md](research/paper/r48-main-manuscript.md) |
| **R47 follow-up evidence report** | [docs/r47-final-evidence-report.md](docs/r47-final-evidence-report.md) |
| **R47 experiment index** | [research/experiments/r47-discrimination/README.md](research/experiments/r47-discrimination/README.md) |
| **Presentation outline** | [research/presentation/outline.md](research/presentation/outline.md) |

### Research — historical AROS drafts (R4.0–R4.3)

These remain for provenance; they are **not** the current publication manuscript:

- [R4.3 final report](docs/r43-final-report.md) (historical)
- [R4.3 active-apply audit](docs/r43-active-apply-audit.md) (historical)
- [R4.3 paper draft](research/paper/main.md) (historical draft; superseded by the R4.5 manuscript above)
- [R4.3 external reproduction notes](research/reproduction/r43-external-reproduction.md) (prior reproduction path)

## License

[MIT](LICENSE) — version [1.0.0](VERSION)
