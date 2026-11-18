# R3.3 Shadow Mode

Default. Collect → analyze → decide → safety probe → report.

Does **not**:

- Call `haeun_recfg_commit`
- Write `0xA474`
- Mutate PCB / timer / telemetry

Safe to run repeatedly.

## Execute (opt-in)

`--plan-policy --execute` sets shadow=0 only when `--execute` is present without forcing shadow. Implementation: `haeun_plan_cli_run(..., execute)`.

Still requires R3.1 PASS preconditions and uses R3.2 only.
