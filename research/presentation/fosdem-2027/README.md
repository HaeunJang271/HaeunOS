# HaeunOS 1.0 — FOSDEM 2027 Kernel Devroom materials

Presentation track target: **FOSDEM Kernel Devroom** (systems / research OS demo).
This directory is for **HaeunOS Release 1.0** (VM + process isolation + desktop/SDK).
It is **not** a replacement for the separate AROS research package under `research/presentation/outline.md` and tag `v4.6.0`.

## Accurate product framing

| Claim | Status |
|-------|--------|
| Custom HAEUN-32 guest ISA | Yes |
| Guest code runs in host C17 interpreter (`src/vm.c`) | Yes |
| Modular monolithic guest research OS | Yes |
| MMU/TLB + process-v1 isolation | Yes (scoped research VM) |
| User-process fault containment (fatal → ZOMBIE) | Yes (see `docs/fault-handling-v1.md`) |
| Desktop shell + external HXF SDK | Yes (documented caps) |
| Microkernel | **No** — do not describe HaeunOS as a microkernel |
| Bare-metal / PC-bootable OS | **No** |
| Related to Amiga AROS | **No** (acronym coincidence only) |
| Physical HID validation complete | **No claim** unless a filled manual checklist exists |

## Directory inventory

| Path | Status |
|------|--------|
| `README.md` | Present (this file) |
| `demo-checklist.md` | Present |
| `evidence/README.md` | Present (pointers only; no fabricated results) |
| `slides.pptx` | **MISSING** — no verified FOSDEM Kernel Devroom deck in the workspace |
| `speaker-script-ko-en.docx` | **MISSING** — no verified FOSDEM speaker script in the workspace |
| `qa-ko-en.docx` | **MISSING** — no verified FOSDEM Q&A document in the workspace |

Do **not** copy the R4.8 AROS talk (`HaeunOS_AROS_R4.8_*`) into this folder: that deck is a different research story (policy-word safety / SHADOW·APPLY).

## Suggested demo commands (Windows)

```powershell
make all
.\build\mmu-tests.exe
.\build\tlb-tests.exe
python .\tests\m17_process_fault_tests.py .\build\haeun-vm.exe
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
```

Desktop live interaction checklist: [docs/m22.4-manual-validation.md](../../../docs/m22.4-manual-validation.md).
Recorded automation or PPM dumps are **not** physical HID PASS.

## Related AROS research (do not overwrite)

- Talk outline: [../outline.md](../outline.md)
- Manuscript R4.8: [../../paper/r48-main-manuscript.md](../../paper/r48-main-manuscript.md)
- Frozen package: tag `v4.6.0`
