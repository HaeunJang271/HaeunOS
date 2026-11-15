# M19 Phase 0 — SDK Audit

| Area | Status | Notes |
|------|--------|-------|
| HAEUN ISA | IMPLEMENTED | v1–v4; SDK emits ISA 4 only |
| HXF binary format | IMPLEMENTED | `tools/build_app_hxf.py` + `haeun_toolchain` |
| Assembler | IMPLEMENTED | `tools/haeun_toolchain.py` |
| HXF build scripts | IMPLEMENTED | `build_app_hxf`, `build_process_*` |
| Userland calling convention | IMPLEMENTED | r1=ecall#, args r2.., ret r1, link r15 |
| `userland/libapp.s` | IMPLEMENTED | exit/pid/yield/wait/win_*/compose/count/spawn/putc/open/read/write/close |
| ECALL wrappers | IMPLEMENTED | via libapp (reused, not duplicated) |
| App startup / exit | IMPLEMENTED | `user_init` @ 0x00108000; EXIT→ZOMBIE |
| Graphics API | PARTIAL | create/fill/pixel/raise/destroy/compose; no DRAW_TEXT |
| Keyboard / mouse API | NOT IMPLEMENTED | no POLL_EVENT in SDK surface |
| HFS filesystem API | IMPLEMENTED | path OPEN assist + byte R/W/CLOSE |
| App execution | IMPLEMENTED | `--apps` builtins; `--app` + `process_sdk` |
| Makefile / Python tools | IMPLEMENTED | `make sdk`, `package_sdk.py`, `haeun.py` |
