# FOSDEM evidence captures

Verified PNG captures used by the FOSDEM 2027 Kernel Devroom deck live in this folder.
They are also embedded (possibly recompressed) in `../HaeunOS_FOSDEM_5min_Talk_Revised.pptx` (slides 4–5).

## Files in this directory

| File | Size / pixels | What it shows |
|------|---------------|---------------|
| `m17-test-pass.png` | 774×43 | `python tests/m17_process_fault_tests.py build/haeun-vm.exe` → `m17 process fault tests passed` |
| `process-fault-output.png` | 698×32 | `build/haeun-vm.exe build/process_fault.hxf` → guest halt after 24231 instructions |
| `desktop-framebuffer-320x180.png` | **320×180** | HAEUN-OS desktop framebuffer (Welcome + Calculator / Editor / Files) |

## External capture tree (optional, outside this Git repository)

Author-host Windows capture directory (optional; sibling of the clone, outside this Git repository):

`../HaeunOS-FOSDEM-Evidence/capture_20261120_025847/`

That capture may include fuller logs (MMU/TLB, desktop smoke, SDK workflow). Treat it as **local evidence**, not as a substitute for:

- a filled physical HID checklist in [docs/m22.4-manual-validation.md](../../../../docs/m22.4-manual-validation.md)
- a final OBS recording

## Do not claim from incomplete evidence

| Observation | Valid claim |
|-------------|-------------|
| Apps registered in logs | Registration occurred |
| Viewer `frames=N` | Viewer path drew frames |
| PPM / 320×180 PNG dump | Framebuffer snapshot after N steps |
| m22.3 scripted tests | Scripted guest input — **not** physical HID |
| Blank Result cell left empty by an operator | Remains **NOT TESTED** until filled |
| Current filled Win32 manual checklist | **9 PASS** — see [docs/m22.4-manual-validation.md](../../../../docs/m22.4-manual-validation.md); demo GIF is partial visual evidence only |
