# Desktop v2 (M20)

## What changed vs M12/M14

- Per-process input demux + `EVENT_POLL`
- `WIN_DRAW_TEXT` for in-window ASCII
- IPC + service name registry
- App HXF relocation fix (`build_app_hxf`) so SDK/built-in apps execute without illegal-instruction faults

## Running

```text
make all
build/haeun-vm.exe build/process_sdk.hxf --app build/app_m20_text.hxf
build/haeun-vm.exe build/process_apps.hxf --apps --viewer
```

Interactive dual-app focus still uses chrome + viewer; scripted checks are in `tests/m20_*.`.

## Not a new package system

Apps still register via `--app` / `--apps` HXF images (M14/M19).
