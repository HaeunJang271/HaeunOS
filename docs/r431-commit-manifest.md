# R4.3.1 Final Pre-Commit Manifest

**Verdict: READY_FOR_APPROVAL**  
**HEAD:** `5563a8614820bb73f76e4ce683e339dc78f0e4c6`  
**Git mutations:** none (`add`/`commit`/`reset`/`clean`/`push` not run)  
**Proposed message:** `research: freeze HaeunOS R4.3 validation baseline`

## Counts

| Set | Count |
|-----|-------|
| Tracked modified (INCLUDE) | **14** |
| Untracked (all) | **205** |
| Untracked INCLUDE | **193** |
| Untracked EXCLUDE | **12** |
| Proposed commit paths | **207** |
| Tracked diffstat | **+435 / −60** |

---

## 1. Tracked modifications (all INCLUDE)

| Path | Class | Purpose / main areas | R4.3 need | If excluded |
|------|-------|----------------------|-----------|-------------|
| `Makefile` | build | R33–R43 bins + make test hooks | Required | R4.3 재현 실패/의미 왜곡 |
| `README.md` | docs | Research doc links | Recommended | R4.3 재현 실패/의미 왜곡 |
| `docs/process-v1.md` | docs | PCB ready_seq / MMIO map | Recommended | R4.3 재현 실패/의미 왜곡 |
| `include/haeun/policy_verify.h` | host | ready_seq fixture + sched trace | Required | R4.3 재현 실패/의미 왜곡 |
| `include/haeun/pval_types.h` | host | ready_seq snapshot fields | Required | R4.3 재현 실패/의미 왜곡 |
| `kernel/process/proc_fork.s` | kernel | telem_child before READY publish | Required | R4.3 재현 실패/의미 왜곡 |
| `kernel/process/proc_policy.s` | kernel | FIFO by 64-bit ready_seq | Required | R4.3 재현 실패/의미 왜곡 |
| `kernel/process/proc_telem.s` | kernel | ready_seq_take + wrap 0xA478 | Required | R4.3 재현 실패/의미 왜곡 |
| `src/app_env.c` | host | host spawn ready_seq mirror | Required | R4.3 재현 실패/의미 왜곡 |
| `src/main.c` | host | planner/feedback/opt/robust CLI | Required | R4.3 재현 실패/의미 왜곡 |
| `src/policy_verify.c` | host | host RR/FIFO+seq mirror | Required | R4.3 재현 실패/의미 왜곡 |
| `src/pval_snapshot.c` | host | snapshot ready_seq fields | Required | R4.3 재현 실패/의미 왜곡 |
| `src/pval_timestamp.c` | host | seq/wrap validation | Required | R4.3 재현 실패/의미 왜곡 |
| `tests/r31_pval_unit_tests.c` | tests | updated pval expectations | Required | R4.3 재현 실패/의미 왜곡 |

### Critical detail

- **Makefile:** R33–R43 unit/guest/integration binaries + `make test` wiring.
- **main.c:** CLI for planner/feedback/opt/robust modes.
- **policy_verify.*:** host mirror prefers ready_seq when present.
- **pval_* / app_env.c:** ready_seq / wrap; host spawn allocates seq.
- **Kernel:** R3.4 ready_seq — **WT ≠ Git HEAD**, **WT == R4.3 baseline**.

---

## 2. Kernel integrity (three-way)

| File | Git HEAD SHA-256 | Working tree / R4.3 | HEAD==WT | WT==R43 |
|------|------------------|---------------------|----------|---------|
| proc_sched.s | f378293d24ef74fe9af42615959e40c5c1de038d49a38590adfa562bf200d7d7 | same | YES | YES |
| proc_policy.s | **73469ee95afa1ab6c6a2e408fe219e618bc16c36d0f23de440c2c152c2d20e82** | 38a79c49d2f9f9d72589f2c13e63f96c1b279bb051e02ac8bb1328f7d81299de | **NO** | YES |
| proc_trap.s | e6cb2a9823a5e6a4d3bcba1322fcaad4e4732fb3138972df7cef672ebe3bc020 | same | YES | YES |
| proc_telem.s | **ad9369419b2a19118e8481a98e638a0c33b0803e6a749de2da46c5d87c7464d4** | 2d7dbeacdb4326838487a580f946de660a39c141c408e6767d529a89c06a705a | **NO** | YES |
| proc_fork.s | **02e6842e253b498c312b0a2a8738fc5a75169c6a1760589abef980129e295281** | d895e560f52a33610fb9f63271d854bb38d7941422bc9a4eb093f57ed60ca603 | **NO** | YES |

### Accurate statements

1. **R4.3 baseline과 working tree의 커널 해시: 일치한다.**
2. **Git HEAD와 working tree 사이: `proc_policy.s`, `proc_telem.s`, `proc_fork.s`에 커널 변경이 존재한다** (R3.4 ready_seq). `proc_sched.s` / `proc_trap.s`는 HEAD=WT.

### ready_seq (HEAD → WT)

| Before (HEAD) | After (WT / R4.3) |
|---------------|-------------------|
| FIFO key: ready_since (+160), modular | FIFO key: unsigned 64-bit ready_seq (+172/+176) |
| No global seq counter | ready_seq_take at 0xA46C/0xA470; wrap sticky 0xA478 |
| telem_child after READY publish | telem_child **before** READY publish |

---

## 3. Untracked INCLUDE (193)

| Class | Count | Decision |
|-------|-------|----------|
| AROS host `src/{feedback,opt,planner,robust}_*.c` | 34 | INCLUDE |
| headers `include/haeun/{feedback,opt,planner,robust}*` | 9 | INCLUDE |
| tests `tests/r33_*`…`r43_*` | 14 | INCLUDE |
| docs `docs/r33-*`…`r431-*` | 89 | INCLUDE |
| research (file-by-file) | 47 | INCLUDE |

### Exact INCLUDE paths

- `docs/r33-architecture.md`
- `docs/r33-decision-model.md`
- `docs/r33-limitations.md`
- `docs/r33-planner.md`
- `docs/r33-pre-audit.md`
- `docs/r33-safety-integration.md`
- `docs/r33-shadow-mode.md`
- `docs/r33-test-report.md`
- `docs/r34-active-validation.md`
- `docs/r34-architecture.md`
- `docs/r34-limitations.md`
- `docs/r34-pre-audit.md`
- `docs/r34-ready-ordering.md`
- `docs/r34-scheduler-trace.md`
- `docs/r34-test-report.md`
- `docs/r34-transaction.md`
- `docs/r35-adaptive-controller.md`
- `docs/r35-architecture.md`
- `docs/r35-evaluation-methodology.md`
- `docs/r35-feedback-model.md`
- `docs/r35-limitations.md`
- `docs/r35-metrics.md`
- `docs/r35-pre-audit.md`
- `docs/r35-state-persistence.md`
- `docs/r35-test-report.md`
- `docs/r36-architecture.md`
- `docs/r36-experiment-design.md`
- `docs/r36-limitations.md`
- `docs/r36-objective-model.md`
- `docs/r36-optimizer.md`
- `docs/r36-pareto-analysis.md`
- `docs/r36-pre-audit.md`
- `docs/r36-safety.md`
- `docs/r36-statistics.md`
- `docs/r36-test-report.md`
- `docs/r37-architecture.md`
- `docs/r37-experiment-design.md`
- `docs/r37-limitations.md`
- `docs/r37-pre-audit.md`
- `docs/r37-reconfiguration-stress.md`
- `docs/r37-reproducibility.md`
- `docs/r37-results.md`
- `docs/r37-robustness.md`
- `docs/r37-sensitivity.md`
- `docs/r37-statistics.md`
- `docs/r37-test-report.md`
- `docs/r40-claim-evidence-matrix.md`
- `docs/r40-component-boundaries.md`
- `docs/r40-data-flow.md`
- `docs/r40-experiment-protocol.md`
- `docs/r40-experiment-results.md`
- `docs/r40-hypotheses.md`
- `docs/r40-limitations.md`
- `docs/r40-overhead-analysis.md`
- `docs/r40-release-notes.md`
- `docs/r40-research-audit.md`
- `docs/r40-safety-model.md`
- `docs/r40-state-machine.md`
- `docs/r40-statistical-analysis.md`
- `docs/r40-system-architecture.md`
- `docs/r41-contributions.md`
- `docs/r41-experiment-design.md`
- `docs/r41-experiment-limitations.md`
- `docs/r41-experiment-results.md`
- `docs/r41-limitations.md`
- `docs/r41-pre-audit.md`
- `docs/r41-recommendation-verification.md`
- `docs/r41-release-notes.md`
- `docs/r41-research-differentiation.md`
- `docs/r41-statistical-analysis.md`
- `docs/r42-baseline-audit.md`
- `docs/r42-evidence-audit.md`
- `docs/r42-final-report.md`
- `docs/r42-kernel-integrity.md`
- `docs/r42-performance-analysis.md`
- `docs/r42-profile-verification.md`
- `docs/r42-reproduction-audit.md`
- `docs/r43-active-apply-audit.md`
- `docs/r43-baseline-audit.md`
- `docs/r43-claim-audit.md`
- `docs/r43-external-package-audit.md`
- `docs/r43-final-report.md`
- `docs/r43-git-baseline.md`
- `docs/r43-kernel-integrity.md`
- `docs/r43-policy-selection-audit.md`
- `docs/r43-worktree-audit.md`
- `docs/r431-commit-audit.md`
- `docs/r431-commit-manifest.md`
- `docs/r431-worktree-reproduction.md`
- `include/haeun/feedback.h`
- `include/haeun/feedback_experiment.h`
- `include/haeun/feedback_types.h`
- `include/haeun/opt_types.h`
- `include/haeun/optimize.h`
- `include/haeun/planner.h`
- `include/haeun/planner_types.h`
- `include/haeun/robust.h`
- `include/haeun/robust_types.h`
- `research/evidence/r42-claims-registry.json`
- `research/evidence/r42-claims-registry.md`
- `research/evidence/r43-claims-registry.json`
- `research/evidence/r43-claims-registry.md`
- `research/experiments/r42-performance-results.json`
- `research/experiments/r42-performance-results.md`
- `research/experiments/r42-profile-verification.json`
- `research/experiments/r42-profile-verification.md`
- `research/experiments/r42-statistics.json`
- `research/experiments/r43-active-apply.json`
- `research/experiments/r43-profile-apply-mapping.json`
- `research/experiments/r43-rollback-verification.json`
- `research/experiments/r43-safety-rejection.json`
- `research/experiments/r43-selection-trace.json`
- `research/experiments/r43-selection-verification.md`
- `research/figures/aros-architecture.md`
- `research/figures/claim-evidence.md`
- `research/figures/experiment-methodology.md`
- `research/figures/feedback-loop.md`
- `research/figures/host-kernel-boundary.md`
- `research/figures/optimization-pipeline.md`
- `research/figures/reconfig-state-machine.md`
- `research/paper/main.md`
- `research/presentation/outline.md`
- `research/presentation/reviewer-questions.md`
- `research/related-work/comparison-matrix.md`
- `research/related-work/r42-comparison-matrix.md`
- `research/related-work/r42-literature-audit.md`
- `research/related-work/research-notes.md`
- `research/related-work/search-log.md`
- `research/related-work/verified-sources.md`
- `research/reproduction/README.md`
- `research/reproduction/commands.md`
- `research/reproduction/environment.md`
- `research/reproduction/expected-artifacts.md`
- `research/reproduction/r42-environment.template.json`
- `research/reproduction/r42-reproduce.md`
- `research/reproduction/r43-artifact-manifest.json`
- `research/reproduction/r43-baseline.json`
- `research/reproduction/r43-external-reproduction.md`
- `research/reproduction/r43-worktree-reproduction.md`
- `research/reproduction/r43-worktree-results.json`
- `research/reproduction/r431-baseline.json`
- `research/reproduction/run-r42-reproduction.ps1`
- `research/reproduction/run-r42-reproduction.sh`
- `research/reproduction/run-r43.ps1`
- `research/reproduction/run-r43.sh`
- `src/feedback_cli.c`
- `src/feedback_compare.c`
- `src/feedback_controller.c`
- `src/feedback_experiment.c`
- `src/feedback_metrics.c`
- `src/feedback_report.c`
- `src/feedback_state.c`
- `src/feedback_types.c`
- `src/opt_apply.c`
- `src/opt_cli.c`
- `src/opt_experiment.c`
- `src/opt_objectives.c`
- `src/opt_pareto.c`
- `src/opt_report.c`
- `src/opt_select.c`
- `src/opt_state.c`
- `src/opt_stats.c`
- `src/opt_types.c`
- `src/planner_analyze.c`
- `src/planner_cli.c`
- `src/planner_decide.c`
- `src/planner_monitor.c`
- `src/planner_report.c`
- `src/planner_run.c`
- `src/planner_safety.c`
- `src/planner_types.c`
- `src/robust_cli.c`
- `src/robust_reconfig.c`
- `src/robust_replay.c`
- `src/robust_report.c`
- `src/robust_runner.c`
- `src/robust_sensitivity.c`
- `src/robust_types.c`
- `src/robust_verify.c`
- `tests/r33_planner_guest_tests.c`
- `tests/r33_planner_unit_tests.c`
- `tests/r34_active_guest_tests.c`
- `tests/r34_active_unit_tests.c`
- `tests/r35_feedback_guest_tests.c`
- `tests/r35_feedback_unit_tests.c`
- `tests/r36_opt_guest_tests.c`
- `tests/r36_opt_unit_tests.c`
- `tests/r37_robust_guest_tests.c`
- `tests/r37_robust_unit_tests.c`
- `tests/r40_integration_tests.c`
- `tests/r41_validation_tests.c`
- `tests/r42_validation_tests.c`
- `tests/r43_active_verification_tests.c`

---

## 4. Untracked EXCLUDE (12)

Exact EXCLUDE paths (temporary logs + absolute-path env/manifest):

- `build_all.log`
- `build_all_r35.log`
- `build_all_r36.log`
- `build_all_r37.log`
- `build_test.log`
- `build_test_r35.log`
- `build_test_r36.log`
- `build_test_r37.log`
- `research/reproduction/r42-environment.json`
- `research/reproduction/r42-manifest.json`
- `research/reproduction/r42-repro-console.log`
- `research/reproduction/r42-reproduction-run.log`

Also never commit: `build/**`, `*.exe`, `dist/` (gitignore).

---

## 5. Reproducibility dependency audit

| Dependency | Status |
|------------|--------|
| HXF in git | Not required — generated via tracked `tools/build_r*.py` + `userland/r*.s` |
| Workload sources / build scripts | **Already tracked** |
| Makefile R33–R43 targets | INCLUDE via Makefile |
| Experiment JSON | INCLUDE under `research/experiments/` |
| Compiler / Python | C17 + Python 3 (documented) |
| External libs | Existing WinSock/GDI on Windows |
| Env vars | None required for core `make test` |
| Sensitive env JSON | EXCLUDE; template `r42-environment.template.json` INCLUDE |

**Missing after proposed freeze:** none for same-OS rebuild/test.  
**Still NOT_TESTED:** detached HEAD worktree / third-party lab (post-commit).

---

## 6. Sensitive information

- INCLUDE rescanned: no absolute user-home paths in payload files.
- EXCLUDE holds absolute-path JSON/logs only.
- Replacement: `research/reproduction/r42-environment.template.json`.

## 7. Risks

| Risk | Mitigation |
|------|------------|
| Freezing kernel diffs vs HEAD | Disclose as R3.4 ready_seq matching R4.3 baseline |
| Large docs/research commit | Acceptable for research freeze |
| Accidental EXCLUDE skip | Stage by explicit path list only |

## 8. Proposed commit message

```
research: freeze HaeunOS R4.3 validation baseline
```
