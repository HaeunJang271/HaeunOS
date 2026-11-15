# Application Lifecycle

1. Host loads `process_sdk.hxf` (process-v1 kernel + SDK runner).
2. `--app path.hxf` registers each app image (name = file stem).
3. Runner calls `APP_COUNT` / `APP_SPAWN` / `WAIT` for each index.
4. Child `user_init` runs at VA `0x00108000` with a private window/file table.
5. Child calls `EXIT` (`app_exit`); host reaps; runner continues or exits.

Calling convention for `libapp` wrappers: arguments in `r2..`, return in `r1`, link in `r15` (`jal r15, name`).
