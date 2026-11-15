# Applications (M14 / Release 1.0)

## Model

- Host registers named HXF images (`calc`, `edit`, `files`) from `build/app_*.hxf`.
- Guest launcher uses `APP_COUNT` / `APP_NAME` / `APP_SPAWN` (ECALL 24–26).
- M20.5 GUI launcher (`process_launch.hxf`) also uses `APP_INFO` / `APP_RUN` / `APP_CTRL` (38–40).
- Each spawn uses a free AS slot (max two concurrent apps + init).
- App code ≤ 4KiB at VA `0x00108000`.

## Build

```sh
python tools/build_app_hxf.py userland/app_calc.s build/app_calc.hxf
python tools/build_process_apps.py build/process_apps.hxf
build/haeun-vm.exe build/process_apps.hxf --apps
```

## Included apps

| Name | Source | Role |
|---|---|---|
| calc | `userland/app_calc.s` | Demo calculator window |
| edit | `userland/app_edit.s` | Demo editor |
| files | `userland/app_files.s` | Demo file browser over HFS |

## Limits

No package install, no dynamic linking, no HFS path EXEC. Details: [apps-v1.md](apps-v1.md).
