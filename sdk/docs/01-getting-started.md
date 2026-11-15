# Getting Started

```text
python haeun.py new my-app
cd my-app
python ..\haeun.py build
python ..\haeun.py verify build\app.hxf
python ..\haeun.py run build\app.hxf
```

(From a packaged SDK directory; adjust the relative path to `haeun.py`.)

Expected run markers:

- `apps: registered app`
- `app-env: spawn app`
- `guest halted`

## Minimal `src/app.s`

```asm
.isa 4
.global user_init

user_init:
    addi r2, r0, 0
    jal r15, app_exit
```

`user_init` must be the first instruction in `.text`. The SDK links `lib/libapp.s` automatically when `haeun.json` has `"libapp": true`.
