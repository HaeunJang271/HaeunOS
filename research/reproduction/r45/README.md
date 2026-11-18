# R4.5 Reproducibility Package

**Third-party reproduction status: NOT TESTED**  
**Do not claim independent lab success from this package alone.**

## Immutable commits

| Role | SHA |
|------|-----|
| Implementation freeze (R4.3.1) | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| R4.3.2 reproduction docs | `735013eccec1f48f814245082238b94c04f4e288` |
| R4.4 research audit docs | `3de73d4ee0649d990bc0da5904872f24767fe53e` |

Prefer checking out / worktree of **`386c3a1…`** for build/test. Docs tips are documentation-only descendants.

## Environment requirements (author host observed)

- OS: Windows 10 build 22631 (other hosts NOT TESTED)
- `cc` C17 (MinGW 15.2.0 observed)
- GNU Make 4.4.1
- Python 3.x (image builders)
- Default flags: `-std=c17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude`
- Windows link libs for VM: `user32`, `gdi32`, `ws2_32`

Sanitized env template: `../r42-environment.template.json` (do not commit filled absolute-path copies).

## Build and test

```text
git worktree add --detach ../HaeunOS-r45-wt 386c3a1517c49be8320c6ea263e726a6c006bb53
cd ../HaeunOS-r45-wt
make clean && make all && make test
```

Author evidence for this chain: `docs/r432-reproduction-evidence.md`  
Exit labeling: `CHAIN_EXIT_0; INDIVIDUAL_EXIT_CODES_NOT_CAPTURED`

## Artifact index (existing; do not regenerate as “new primary”)

See `artifact-index.md`.

## Expected interpretation

- Suite pass lines (r41/r42/r43) support engineering reproducibility on a similar toolchain.
- 61/61 applies only to recorded R4.3 behavioral mirror evidence.
- Descriptive R4.2 tables are not statistical proof.
- EXCLUDE absolute-path env/logs must not be published as freeze inputs.

## Limitations

- Same-machine evidence ≠ third-party.
- Worktree may become artifact-dirty after `make test`.
- Two policies only; 30-cell live APPLY not tested per cell.
