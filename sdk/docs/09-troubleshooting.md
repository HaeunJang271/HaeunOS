# Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| `missing user_init` | Symbol not `.global` or not first in `.text` |
| `code exceeds 4KiB` | Trim code; do not ship large data blobs |
| `entry must be 0x00108000` | Hand-edited HXF / wrong builder |
| `haeun-vm not found` | `make` then `make sdk`, or set `HAEUN_VM` |
| `register … failed` | Bad HXF; run `haeun verify` |
| App never exits | Missing `app_exit` / EXIT ecall |
| Wrong colors only | No text API — use fill/pixel as status |
