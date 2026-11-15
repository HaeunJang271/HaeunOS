# Services v1

## IPC name registry (M20.2)

Minimal **name → IPC endpoint** table (host).

| ECALL | # | Args |
|-------|---|------|
| SVC_REGISTER | 33 | r2=name VA r3=epid |
| SVC_LOOKUP | 34 | r2=name VA → epid |

Duplicate names rejected. Owner pid recorded; EXIT/fault reaps registrations.

## Process Service Manager (M20.6)

Separate from 33–34. Starts **userland HXF apps** as managed services.

| ECALL | # |
|-------|---|
| SERVICE_COUNT | 41 |
| SERVICE_INFO | 42 |
| SERVICE_START | 43 |
| SERVICE_STOP | 44 |
| SERVICE_STATUS | 45 |
| SERVICE_RESTART | 46 |

See [m20-service-manager-report.md](m20-service-manager-report.md).
