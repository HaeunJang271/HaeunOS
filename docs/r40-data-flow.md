# R4.0 Data Flow

```mermaid
sequenceDiagram
  participant G as Guest
  participant V as Host VM
  participant T as Telem/Eval
  participant O as Opt/Robust
  participant S as R3.1
  participant R as R3.2
  G->>V: instructions / traps
  V->>G: haeun_vm_step
  T->>G: read PCB/policy (RO)
  O->>T: metrics samples
  O->>O: score / Pareto / recommend
  Note over O,R: APPLY explicit only
  S->>G: snapshot seal (RO)
  R->>G: write 0xA474 at between-step
  R->>S: re-check / verify
```

## Knowledge base (host state)

- Current observed policy ID
- Candidate results (RR/FIFO)
- Experiment history (`haeun_rb_experiment`)
- Profile weights / config hash
- Validation reports
- Recommendation + verify status
- Feedback history (R3.5)

Guest RAM is not the knowledge base; host structs/JSON are.
