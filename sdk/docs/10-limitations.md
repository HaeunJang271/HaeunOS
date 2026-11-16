# SDK Limitations (v1)

- No high-level language compiler (ASM + existing assembler only)
- 4KiB max app code segment
- Nested HFS paths unsupported
- Concurrent app slots limited by process-v1 AS (init + 2 apps)
- ISA / HXF / ECALL ABIs are frozen; do not fork the kernel for apps
- Default FB **320×180**; ASCII 8×8 text only
- `WIN_DRAW_TEXT` / `EVENT_POLL` are available via `libapp.s` helpers; interactive UI is **not** fully covered by SDK automated tests
- Live keyboard/mouse need host `--viewer` / `--input`; physical HID is not separately certified

See tree docs: `docs/limitations.md`, `docs/graphics-api.md`.
