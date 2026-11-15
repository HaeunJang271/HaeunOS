# SDK Limitations (v1)

- No high-level language compiler (ASM + existing assembler only)
- 4KiB max app code segment
- No `WIN_DRAW_TEXT` / `POLL_EVENT` in the public API
- Nested HFS paths unsupported
- At most `HAEUN_APP_MAX` (4) `--app` registrations per VM; at most 2 concurrent app slots (+ launcher)
- Keyboard/mouse demos require host `--input` and are outside this SDK surface
- ISA / HXF / ECALL ABIs are frozen; do not fork the kernel for apps
