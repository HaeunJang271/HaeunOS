# R3.5 State Persistence

Host-side only (`haeun_fb_state`). Not guest RAM.

## Format

Deterministic JSON with `schema_version` (`HAEUN_FB_SCHEMA=1`).

Fields: controller_version, cycle_id, mode, policies, previous decision/outcome,
fingerprint, measurement_generation, cooldown, consecutive counters, history[], cfg.

## API

| Call | Behavior |
|------|----------|
| `haeun_fb_state_save` | write JSON |
| `haeun_fb_state_load` | parse; incompatible schema → `BAD_STATE` |
| `haeun_fb_state_validate` | schema/bounds check |

Malformed / unknown schema is rejected. No silent accept.
