# HXF Format (SDK apps)

Application HXFs produced by `haeun build`:

| Field | Value |
|-------|--------|
| Magic | `HXF1` |
| ISA | 4 |
| Entry / load VA | `0x00108000` |
| Segments | one R\|W\|X code segment |
| Max code | 4096 bytes |

`haeun verify` rejects wrong magic, ISA, entry, missing RX segment at user VA, or size > 4KiB.
