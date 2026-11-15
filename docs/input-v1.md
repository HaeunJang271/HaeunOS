# Guest keyboard & mouse input v1 (M10)

Status: **board MMIO ABI is preserved**; host feeds scancodes/samples through existing VM APIs. Scripted host injection (`--input`) drives automated demos. Real OS keyboard/mouse capture is optional and not required for M10 completion.

## Board contract (unchanged)

See [architecture/memory-map.md](architecture/memory-map.md):

| Device | Base | Host API |
|---|---|---|
| Keyboard | `0x10005000` | `haeun_vm_keyboard_feed` |
| Mouse | `0x10006000` | `haeun_vm_mouse_post` |

## Scancode convention

| Event | Byte |
|---|---|
| Key make (press) | `code & 0x7F` |
| Key break (release) | `code \| 0x80` |

Example: `0x1C` press, `0x9C` release. Layout/ASCII mapping is out of scope for v1.

Keyboard RX is a 64-byte ring. `haeun_vm_keyboard_feed` rejects atomically when the batch would overflow (returns false; FIFO unchanged).

## Mouse samples

One pending sample: `X`, `Y` (signed screen pixels), `BUTTONS` bits 0–2 (L/R/M).

- A new `haeun_vm_mouse_post` **overwrites** any unread sample and sets pending.
- Guest clears pending by writing ACK (`+0x14`).

## IRQ policy (process-v1)

IRQC sources 3 (keyboard) and 4 (mouse) exist, but process-v1 trap only acknowledges timer (cause 16). Enabling KBD/MOUSE RX IRQ under that trap risks mis-dispatch.

`--input` / `haeun_vm_input_host_attach` keeps `kbd_control` and `mouse_control` at 0 (polling only). Guests should poll STATUS.

## GUI focus routing

When a WM is attached, a left-button mouse sample is hit-tested top-down by z-order. The hit window is raised and becomes `focus_id`. Misses clear focus (`focus_id = -1`).

## Guest demo

```text
python tools/build_input_demo.py build/input_demo.hxf
./build/haeun-vm build/input_demo.hxf --gui --user --input
```

## Not claimed

- Live host OS keyboard/mouse capture in the default CLI
- Per-process input queues / EXIT cleanup of board FIFOs
- New input ECALLs (ABI remains MMIO)
