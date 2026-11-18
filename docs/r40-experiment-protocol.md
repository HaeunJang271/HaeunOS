# R4.0 Experiment Protocol

## Catalog

Workloads: cpu_bound, short_job, mixed, bursty, block_sleep, block_wait  
Policies: RR (0), Preemptive FIFO (1)  
Profiles: balanced, fairness, latency, throughput, low_overhead

## Isolation

Each `haeun_eval_run_once` / robust record: fresh RAM + HXF load.  
Config hash + workload seed (FNV over image/policy/step_cap).  
Failed runs recorded; missing ≠ 0.

## Campaigns (R3.7 presets)

| Preset | Reps | R4.0 |
|--------|------|------|
| smoke | 3 | **executed** by integration test |
| standard | 10 | available; NOT required for DoD |
| extended | 30 | NOT TESTED in R4.0 default |
| research | 100 | explicit only; NOT TESTED by default |

## Runner

Reuse `haeun_rb_run_experiments` + sensitivity + verify.  
Do not duplicate engines. Wrapper: `r40-integration-tests`.
