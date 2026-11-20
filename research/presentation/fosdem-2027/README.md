# HaeunOS 1.0 — FOSDEM 2027 Kernel Devroom materials

Submission target: **FOSDEM Kernel Devroom** (systems / research OS demo; not an acceptance claim).
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
| Manual Win32 checklist (9 steps) | **9 PASS** — authoritative: [docs/m22.4-manual-validation.md](../../../docs/m22.4-manual-validation.md) |
| Demo GIF / physical HID framing | GIF (../../../docs/assets/haeunos-demo.gif) is **partial visual evidence** for steps 1–5 and 8 only. Steps 6, 7, and 9 are checklist PASS on live Win32 `--viewer`; do **not** automatically equate those application-level checks with complete physical HID hardware validation |

## Directory inventory

| Path | Status |
|------|--------|
| `README.md` | Present (this file) |
| `demo-checklist.md` | Present |
| `HaeunOS_FOSDEM_5min_Talk_Revised.pptx` | **Verified** — 6 slides; slide 4 embeds two terminal captures; slide 5 embeds a 320×180 HAEUN-OS framebuffer |
| `HaeunOS_FOSDEM_5min_Script_KO_EN_Final.docx` | **Verified** — bilingual 5-minute speaker script (EN spoken / KO practice) |
| `HaeunOS_FOSDEM_5min_Script_KO_EN_Final.pdf` | **Verified** — PDF of the bilingual 5-minute speaker script (4 pages) |
| `HaeunOS_FOSDEM_QA_10min_KO_EN_Final_checked.pdf` | **Verified** — bilingual Q&A guide (checked edition PDF; **4 pages**) |
| `HaeunOS_FOSDEM_QA_10min_KO_EN_Final_checked.docx` | **Verified** — DOCX of the checked bilingual Q&A guide (same edition as the PDF) |
| `evidence/` | **Verified** — three capture PNGs plus `evidence/README.md` (see below) |
| `../../../docs/assets/haeunos-demo.gif` | **Verified** — short local Win32 viewer demo; visual coverage of checklist steps 1–5 and 8 only (**not** proof of all nine steps). Full checklist: **9 PASS** in `docs/m22.4-manual-validation.md` |

### PPTX evidence check (local inspection)

| Slide | Embedded media | Content check |
|-------|----------------|---------------|
| 4 — When a user process faults | `image1.png` (774×43), `image2.png` (698×32) | Terminal: `m17 process fault tests passed`; `build/process_fault.hxf` halt line |
| 5 — What has been tested | `image3.png` (320×180) | HAEUN-OS desktop framebuffer (Welcome / Calculator·Editor·Files) |

On-disk evidence counterparts (same visual content; PPT may recompress bytes):

- `evidence/m17-test-pass.png`
- `evidence/process-fault-output.png`
- `evidence/desktop-framebuffer-320x180.png`

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
