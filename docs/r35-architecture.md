# R3.5 Architecture

Host-only adaptive feedback loop on top of R3.3–R3.4.

```
Monitor → Analyzer → Planner(R3.3) → Feedback gates
  → R3.1 → R3.2 → (optional) continue guest → Sample after
  → Feedback Evaluator → State/History → next cycle
```

## Modes

| Mode | Flag | Commit |
|------|------|--------|
| SHADOW (default) | `--feedback-report` | never |
| ADAPTIVE | `--adaptive` | via R3.2 after fresh R3.1 |
| ADAPTIVE dry-run | `--adaptive --dry-run` | never |

## Modules

| Module | Role |
|--------|------|
| `feedback_metrics` | sample with explicit VALID/UNAVAILABLE/INVALID |
| `feedback_compare` | multi-objective before/after |
| `feedback_controller` | cycle + gates + R3.2 exec |
| `feedback_state` | deterministic JSON save/load |
| `feedback_experiment` | fixed RR / fixed FIFO / Adaptive harness |
| `feedback_cli` | haeun-vm entry |

Kernel scheduling ABI unchanged. No new ECALLs.
