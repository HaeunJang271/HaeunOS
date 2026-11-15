# HAEUN virtual board memory map

Status: **normative for the 256 MiB board profile**. Device semantics are in [platform.md](../platform.md) and the per-device sections below. Guest physical addresses unless paging is enabled.

## System overview

| Component | Size / note |
|---|---|
| Guest RAM | 256 MiB (`0x00000000–0x0FFFFFFF`) |
| Boot firmware reservation | 64 KiB at `0x00000000–0x0000FFFF` (HFW staged at reset) |
| Boot ROM code window | `0x00000000–0x00007FFF` write-protected after HFW load |
| MMIO window | `0x10000000+` device apertures |

## Physical memory map

| Range | Region | Access |
|---|---|---|
| `0x00000000–0x00007FFF` | Boot ROM code (32 KiB) | Execute/read; **writes fault** after firmware staging |
| `0x00008000–0x0000FFFF` | Boot scratch / block DMA buffer | RW |
| `0x00010000–0x000FFFFF` | Low RAM | RW |
| `0x00100000–0x0FEFEFFF` | General RAM (kernel load at `0x00100000`) | RW |
| `0x0FEFF000–0x0FEFFFFF` | Boot-information page (HBI) | RW |
| `0x0FF00000–0x0FFFFFFF` | Initial stack reservation | RW |
| `0x10000000–0x10000FFF` | Interrupt controller | MMIO |
| `0x10001000–0x10001FFF` | System timer | MMIO |
| `0x10002000–0x10002FFF` | UART | MMIO |
| `0x10003000–0x10003FFF` | Block device | MMIO |
| `0x10004000–0x10004FFF` | Framebuffer control | MMIO |
| `0x10005000–0x10005FFF` | Keyboard | MMIO |
| `0x10006000–0x10006FFF` | Mouse | MMIO |
| `0x10007000–0x10007FFF` | Virtual NIC | MMIO |
| elsewhere | Unmapped | Bus fault |

Reset PC is `0x00000000` (Boot ROM / HFW entry). Kernel images load at `0x00100000` via the guest bootloader on the block device ([boot-disk-v1.md](../boot-disk-v1.md)).

## IRQC source IDs

| ID | Device |
|---:|---|
| 0 | Timer |
| 1 | UART RX |
| 2 | Block completion |
| 3 | Keyboard RX |
| 4 | Mouse event |
| 5 | NIC RX |

## Virtual NIC (`0x10007000`)

See [net-v1.md](../net-v1.md) for register layout. Frames use guest-RAM DMA; IRQC source 5 signals RX.

## Framebuffer control (`0x10004000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | STATUS (RO) | bit 0 = configured OK when CONTROL.enable and BASE range valid |
| 0x04 | CONTROL (RW) | bit 0 enable; enable validates BASE against RAM |
| 0x08 | WIDTH (RW) | pixels, 1..1024 |
| 0x0C | HEIGHT (RW) | pixels, 1..1024 |
| 0x10 | BASE (RW) | guest PA of packed RGBA8888 buffer (`width*height*4` bytes) |
| 0x14 | FORMAT (RO) | `1` = RGBA8888 |

Pixels live in guest RAM at BASE. The control aperture does not contain the frame store.

## Keyboard (`0x10005000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | DATA (RO) | next scancode, or bit 31 set if empty |
| 0x04 | STATUS (RO) | bit 0 = byte available |
| 0x08 | CONTROL (RW) | bit 0 RX IRQ enable → IRQC source 3 |

Host feeds scancodes through `haeun_vm_keyboard_feed`. Make/break convention and M10 host scripting: [input-v1.md](../input-v1.md).

## Mouse (`0x10006000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | STATUS (RO) | bit 0 = sample pending |
| 0x04 | X (RO) | signed 32-bit X after last sample |
| 0x08 | Y (RO) | signed 32-bit Y |
| 0x0C | BUTTONS (RO) | bit 0 left, bit 1 right, bit 2 middle |
| 0x10 | CONTROL (RW) | bit 0 IRQ enable → IRQC source 4 |
| 0x14 | ACK (WO) | any write clears pending |

Host posts samples through `haeun_vm_mouse_post`.

## Bootloader contract (guest CPU)

The HFW image at reset must:

1. Execute from the reset vector at `0x00000000`
2. Use Boot ROM / low RAM only for early code and DMA scratch
3. Initialize the block device via MMIO
4. Load and validate an HXF boot image from disk
5. Copy the kernel LOAD segment to its load address
6. Write HBI at `0x0FEFF000`
7. Transfer to the HXF entry with `r2`/`r3`/`r13`/`r14` set

Host CLI must not skip these steps by poking kernel registers in firmware+disk mode ([firmware-boot.md](../firmware-boot.md)).
