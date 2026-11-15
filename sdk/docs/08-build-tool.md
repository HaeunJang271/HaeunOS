# Build Tool Reference

| Command | Action |
|---------|--------|
| `haeun new <name>` | Copy `templates/basic` → project |
| `haeun build [path]` | Assemble → `build/app.hxf` (default `.`) |
| `haeun verify <hxf>` | Structural + SPAWN constraints |
| `haeun run [hxf\|dir]` | `haeun-vm process_sdk.hxf --app …` |

Environment: `HAEUN_VM` overrides VM binary discovery.
