# Project Structure

```text
my-app/
  haeun.json      # main source + lib flags
  src/app.s       # must export user_init
  build/app.hxf   # produced by haeun build
  README.md
```

## `haeun.json`

```json
{
  "main": "src/app.s",
  "libapp": true,
  "libs": []
}
```

- `main`: primary ASM (provides `user_init` at `.text+0`)
- `libapp`: append packaged `lib/libapp.s`
- `libs`: optional extra `.s` files concatenated after `main`
