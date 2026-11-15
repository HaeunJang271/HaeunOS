# Boot disk and firmware HXF load profile v1

Status: **implemented and tested** (multi-block HXF validate/load, HBI construction, transfer to entry). Depends on [firmware-boot.md](firmware-boot.md), [platform.md](platform.md) block MMIO, and [bootinfo-v1.md](bootinfo-v1.md).

## Goal

Let HAEUN firmware, running from an HFW image at reset PC zero, load a guest HXF from a raw block disk, build HBI, and transfer to the HXF entry.

## Disk image

- Host supplies a raw byte array via `haeun-vm --firmware <hfw> --disk <raw>`.
- Length must be a multiple of 512. Block 0 is LBA 0.
- v1 places one HXF image at LBA 0; the image byte length may span multiple blocks and must be padded so the file length is a multiple of 512.

## Firmware sequence (v1)

1. Attach is host-side (`haeun_vm_block_attach`); firmware assumes the block device is present.
2. Read successive 512-byte blocks into a DMA buffer at `0x00008000` (below `0x00010000`) using STATUS/COMMAND/LBA/DMA_ADDR.
3. Validate the HXF header and the single LOAD segment in place (constrained profile below).
4. Copy the LOAD segment to its virtual address (flat VA=PA profile).
5. Encode HBI v1 at `0x0FEFF000`.
6. Set `r2=0x0FEFF000`, `r3=32`, `r13`/`r14` to the board stack bounds, jump to the HXF entry.

On any validation or I/O error: write UART `E\n` and `HALT` with `r1=0`.

## Constrained load profile (v1 firmware)

- Exactly one LOAD segment (`seg_count=1`, `table_offset=64`).
- Declared `file_size` at most `0x8000` (fits the DMA buffer).
- Segment virtual address at least `0x00100000`.
- Segment has the executable flag; entry is 4-byte aligned and lies in the segment.
- Segment `file_size` and BSS gap are multiples of 4 (word copy path).
- ISA version 1..4; header flags zero.

## CLI

```text
haeun-vm --firmware boot.hfw --disk disk.raw
```

Build firmware with the toolchain HFW linker:

```text
python tools/haeun_toolchain.py as firmware/boot.s build/boot.hobj
python tools/haeun_toolchain.py hfw -o build/boot.hfw build/boot.hobj
```

Direct `haeun-vm image.hxf` remains a separate test path that supplies HBI itself.

## Incremental delivery

1. CLI `--disk` attach + firmware that reads LBA 0 and checks `HXF1` magic.
2. Full multi-block HXF validate/load + HBI + transfer (this document).
