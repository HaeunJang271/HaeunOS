# Filesystem API Reference

| Symbol | Args | Notes |
|--------|------|-------|
| `app_open` | r2=1 console **or** r2=path VA, r3≠0 create | → fd |
| `app_read` | r2=fd | → byte or −1 |
| `app_write` | r2=fd r3=byte | |
| `app_close` | r2=fd | |

Path OPEN uses a NUL-terminated root basename in guest memory (see `examples/file_viewer`). Nested paths are not supported in v1.
