# Reproduction Commands

From repository root:

```sh
make clean
make all
make test
```

R4.1 validation alone (after images exist via `make test` deps or stamps):

```sh
make build/r41-validation-tests.exe
./build/r41-validation-tests.exe build both
```

Or smoke only / standard only:

```sh
./build/r41-validation-tests.exe build smoke
./build/r41-validation-tests.exe build standard
```

Alternative via VM CLI (requires a loadable HXF; robustness path uses `--build-dir`):

```sh
./build/haeun-vm.exe build/r12_yield.hxf --build-dir build --robustness-report --experiment-preset smoke --sensitivity-analysis --verify-recommendation --r37-out build/r41-cli-smoke
```

## Troubleshooting

| Symptom | Action |
|---------|--------|
| Missing HXF | Ensure `build/r12-images.stamp` / `r22` / `r30` stamps built |
| Unknown preset | Use `smoke|standard|extended|research` |
| Export IO fail | Use single-level `build/r41-smoke` paths; check permissions |
| Overall FAIL | Inspect `robustness-report.txt`; do not delete failed runs |
