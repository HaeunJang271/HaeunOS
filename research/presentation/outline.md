# HaeunOS AROS — 20-minute research talk (R4.8.25 reframing)

**Primary message:** Recommendation is not authorization. Mutation of a narrow guest policy word requires a host-mediated fail-closed path in a single-threaded research VM.

**Claim hygiene:** Implemented · Tested · Observed · Not tested · Not claimed.

**Hard limits:** two policies only (RR=0, FIFO=1); 61/61 mirror matches are **Observed**; no statistical superiority; third-party reproduction **NOT TESTED**; no SMP/HW atomicity; policy-ID-only rollback (not full VM); no unattended APPLY. Unrelated to Amiga AROS (AROS Research Operating System).

| # | Title | Message | Evidence | Time |
|---|-------|---------|----------|------|
| 1 | Title and research context | HaeunOS AROS on HAEUN-32 research VM; host-assisted evaluate→recommend; safety-gated apply | README; r48 manuscript | 0:45 |
| 2 | Problem and motivation | Evaluating ≠ recommending ≠ mutating guest policy; shared authority blurs the control boundary | Manuscript §1 | 1:30 |
| 3 | Research question and contributions | RQ: fail-closed mutation path distinct from recommendation; C1 separation; C2 host-mediated path; C3 scoped limits | Manuscript §1–2 | 1:30 |
| 4 | System architecture | Custom ISA (not RISC-V); C17 interpreter; modular monolithic guest; host tools read RAM snapshots; policy word `0xA474` | Manuscript §4; historical F1 | 2:15 |
| 5 | Recommendation vs mutation | SHADOW default; APPLY opt-in; recommendation never alone writes policy | Manuscript §4–5; F2 | 2:00 |
| 6 | Prepare / Validate / Commit | Host-mediated prepare≠commit; UNKNOWN≠PASS; single-threaded between-`haeun_vm_step()` safe point; no SMP claim | Manuscript §5 | 2:15 |
| 7 | Safety validation (primary evidence) | R4.3: bidirectional COMMITTED; invalid-target reject before write (request-path `known()`); policy-ID-only rollback; 61/61 mirror **[O]** in logs/audit | r43 artifacts; §7.1 | 2:30 |
| 8 | R4.2 scheduling context | 120/120 runs; six workloads; RR lower ready-wait; Jain equal; inferential **NOT_PERFORMED**; 30/30 RR—descriptive context only | r42 JSON; §7.2 | 1:30 |
| 9 | R47 recommendation limits | Did **not** show workload-dependent discrimination; Gate B measured `NOT_EVALUATED`; C/D1/D2 all `SELECT_RR`; no real FIFO win; W4 discontinued | r47 report; §7.3 | 2:00 |
| 10 | Limitations | Single-threaded scope; no inferential tests; 3rd-party NOT TESTED; policy-ID-only rollback; R47 open limitation | Manuscript §10 | 2:00 |
| 11 | Conclusion and Q&A | Control structure demonstrated in scoped VM; performance/discrimination not primary claims | Manuscript §12 | 1:45 |

**Time total:** 0:45+1:30+1:30+2:15+2:00+2:15+2:30+1:30+2:00+2:00+1:45 = **20:00**.

**Speaker notes (slide 9):** D1 300k step-cap failure (`INSUFFICIENT_TELEMETRY`) ≠ separately approved 800k confirmatory. W4 discontinued, not executed. Gate A unit-test source exists; not independent reproduction.

**Figures (optional):** historical R4.5 F1/F2/F3 under `research/figures/r45/` — architecture aids only; not new experiments.

---

## Korean slide intent (발표 해설)

1. **표지:** 연구의 주인공은 성능 수치가 아니라, 추천과 적용을 가르는 통제 구조다.  
2. **문제:** 평가·추천·변경을 한 권한으로 섞으면 통제 경계가 흐려진다.  
3. **연구 질문·기여:** RQ를 초반에 고정하고 C1–C3(분리 / fail-closed 경로 / 범위 한계)만 약속한다.  
4. **구조:** 호스트 도구 vs 게스트 VM, ISA·인터프리터·정책 워드 위치를 분명히 한다.  
5. **경계:** SHADOW/APPLY로 “추천 ≠ 승인”을 말한다.  
6. **절차:** prepare→validate→commit과 단일 스레드 safe point; SMP/원자성 비주장.  
7. **핵심 증거:** R4.3 적용·거부·정책 ID rollback·61/61은 Observed이며 과장하지 않는다.  
8. **R4.2:** 기술 배경(120 runs, ready-wait, 30/30 RR); 일반 성능 기여로 포장하지 않는다.  
9. **R47:** 추천 차별화 미입증; Gate B NE와 C/D1/D2 SELECT_RR을 섞지 않는다.  
10. **한계:** 단일 스레드, 비추론, 제3자 NOT TESTED, policy-ID rollback.  
11. **결론:** 구조는 보였고, 성능·차별화는 열린 한계다. Amiga와 무관함을 재확인.

## English speaker script (compact)

1. Today’s claim is structural: host-assisted evaluation may recommend, but mutating the guest policy word needs a separate fail-closed path. Unrelated to Amiga AROS.  
2. Evaluating, recommending, and mutating policy are not the same job; conflating them blurs the control boundary.  
3. Research question: can we recommend while requiring fail-closed validation before mutation? Contributions: separation; host-mediated path; scoped limits.  
4. HAEUN-32 is a custom ISA, not RISC-V, run in a C17 interpreter. Modular monolithic guest; host tools read snapshots; policy word at `0xA474`.  
5. SHADOW is default—observe without write. APPLY is explicit opt-in. A recommendation never alone authorizes mutation.  
6. Prepare validates without writing; commit is distinct. Safe point is between interpreter steps on a single-threaded VM—not SMP or hardware atomicity.  
7. Primary evidence is R4.3: bidirectional COMMITTED applies, invalid-target rejection before write (request-path known() check), policy-ID-only rollback, and 61/61 mirror matches in logs—Observed, scoped.  
8. R4.2 is context: 120 successful runs, six workloads, lower RR ready-wait, equal Jain, no inferential tests, 30/30 RR recommendations—not a general performance claim.  
9. R47 did not demonstrate workload-dependent discrimination. Gate B is SELECT_RR with measured NOT_EVALUATED; completed C/D1/D2 cells are SELECT_RR only; W4 was not run.  
10. Limits: single-threaded scope; no inferential testing; third-party NOT TESTED; rollback restores policy ID only.  
11. We demonstrated a host-mediated fail-closed mutation path in a research VM. We do not claim scheduling superiority or recommendation discrimination. Questions.

---

## Historical outline (R4.3 era) — provenance only; not part of the 20-minute talk

Do **not** mix with the R4.8/R47 reframed talk above. Prefer the 20-minute table for external talks.

| # | Title | Message | Evidence | Time |
|---|-------|---------|----------|------|
| 1 | Problem | Unsafe ad-hoc policy change | — | 0:40 |
| 2 | RQ | Host-assisted evaluate→validate→apply | RQ | 0:40 |
| 3 | Existing work | MAPE, Rainbow, K42 | S01–S06 | 1:00 |
| 4 | Architecture | Modular monolith; host vs guest | figures | 1:00 |
| 5 | Host-assisted opt | 5 profiles; SHADOW | r36/r42 | 0:50 |
| 6 | Safety | UNKNOWN≠PASS | r31–r32 | 1:00 |
| 7 | Transactions | Policy-ID rollback | r32/r43 | 1:00 |
| 8 | Selection proof | Mirror vs curr, not ID alone | r43 trace | 1:30 |
| 9 | Active apply | RR↔FIFO COMMITTED + matches | r43 apply | 1:30 |
| 10 | Reproducibility | R4.3-era gaps; 3rd party NOT TESTED | historical notes | 1:00 |
| 11 | Limitations | Catalog; two policies; no SMP | — | 0:50 |
| 12 | Contributions | C2b upgrade; honest gaps | registry | 0:40 |
