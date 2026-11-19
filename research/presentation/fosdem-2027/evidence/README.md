# FOSDEM evidence pointers

This folder does **not** embed binary logs or screenshots. It only documents where verified capture material lives and what is still missing.

## External capture (outside this Git repository)

Author-host Windows capture directory (sibling of the clone):

`../HaeunOS-FOSDEM-Evidence/capture_20261120_025847/`
(absolute example: `C:\Users\level\OneDrive\Desktop\Projects2\HaeunOS-FOSDEM-Evidence\capture_20261120_025847`)

That capture includes (when present on the author machine):

- MMU/TLB and m17 fault-test logs
- Desktop smoke / viewer finite-step logs and a framebuffer PPM
- SDK workflow logs under `sdk/workdir/`

Treat that tree as **local evidence**, not as a substitute for:

- a filled physical HID checklist in [docs/m22.4-manual-validation.md](../../../../docs/m22.4-manual-validation.md)
- a final OBS recording
- FOSDEM-specific `slides.pptx` / speaker / Q&A documents (still **MISSING** in `research/presentation/fosdem-2027/`)

## Do not claim from incomplete evidence

| Observation | Valid claim |
|-------------|-------------|
| Apps registered in logs | Registration occurred |
| Viewer `frames=N` | Viewer path drew frames |
| PPM dump | Framebuffer snapshot after N steps |
| m22.3 scripted tests | Scripted guest input — **not** physical HID |
| Empty Result column in manual checklist | Remains **NOT TESTED** |
