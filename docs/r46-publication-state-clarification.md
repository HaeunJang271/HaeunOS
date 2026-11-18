# R4.6 publication state clarification

## 1. Purpose and scope

This document is **outside** the frozen R4.6 publication package inventory.
It does **not** modify `docs/r46-release-notes.md`, `docs/r46-release-manifest.json`,
historical SHA fields, or `manifest_sha256`.
It exists only to prevent misreading freeze-preparation wording as the live publication state.

It does **not** claim third-party reproduction, new experimental validation, or a GitHub Release object.

## 2. Historical freeze-preparation versus current publication state

| Concept | Meaning |
|---------|---------|
| **A. Freeze-preparation** | Documentation package was ready; commit/tag authorization had not yet been recorded as complete in those files. |
| **B. Approval-pending at write time** | Phrases such as “pending approval” / `awaiting_commit_approval` describe that preparation moment. |
| **C. v4.6.0 publication** | Annotated Git tag `v4.6.0` published the documentation freeze. |
| **D. Current `main` tip** | May advance after the tag with non-package navigation docs. |

Do not conflate A/B with C, or C with D.

## 3. Published annotated Git tag identity

HaeunOS AROS R4.6 was published as an **annotated Git tag**, not as a GitHub Release object.

| Identity | Value |
|----------|-------|
| Tag name | `v4.6.0` |
| Annotated tag object | `e23a8f5e5c328851e4ae6def1683cca3060dff61` |
| Peeled commit (tag target) | `0b7feadc8d77f56c293c04a52e8fb434580f134f` |

Checking out `v4.6.0` yields that peeled commit. This clarification does not move the tag.

## 4. Meaning of pending-approval wording

`docs/r46-release-notes.md` and `docs/r46-release-manifest.json` still contain phrases written during the **historical publication-preparation phase**, for example:

- Release notes: `FINAL_FREEZE_READY (pending explicit commit/tag approval)`
- Release notes: `R4.5–R4.6 documentation files remain uncommitted until approval.`
- Manifest: `"awaiting_commit_approval": true`
- Manifest: `"status": "FINAL_FREEZE_READY"`

**Interpretation rule:** those strings describe states **A/B** (freeze-prep / awaiting approval *when the notes were authored*).
They do **not** mean that commit/tag publication is still pending **after** annotated tag `v4.6.0` exists.
They are retained so the frozen package content stays byte-identical to the published tag tree.

## 5. Historical SHA and provenance interpretation

Fields such as `head_at_audit`, `implementation_baseline_sha`, `evidence_commit_shas`,
`manifest_integrity.frozen_release_commit`, and `pre_rewrite_*` / `post_rewrite_*`
preserve **historical provenance** from earlier audit and rewrite stages.

Do **not** interpret them as:

- the current `main` tip, or
- the `v4.6.0` peel commit (unless a field explicitly names that role).

Example: `frozen_release_commit` = `f1a49c7f75efd2b24129a81576edd5148e6cbc76` is a pre-rewrite freeze identity.
It is **not** the current `main` tip and is **not** the `v4.6.0` peel `0b7feadc…`.

## 6. Manifest self-hash scope

`docs/r46-release-manifest.json` records:

| Field | Value |
|-------|-------|
| `manifest_sha256` | `df8da1d9df682ce3c3135d2d9ffe0402c0dd4548acc98c23df028d8210324244` |
| `manifest_bytes` | `5393` |
| `file_count` / listed package files | `20` |

The self-hash covers the manifest body under the documented canonicalization
(**excluding** keys `manifest_sha256` and `manifest_bytes`).

The inventory lists **20** publication package files. The manifest file itself is the integrity envelope and is **not** one of those 20 listed paths.
This clarification document is **not** part of that inventory and must **not** trigger any rewrite of `manifest_sha256` or `manifest_bytes`.

## 7. R4.7.15 documentation publication

Commit `25b813acaade94271b327168577e74261849ddbe`
(`docs: align public research navigation and presentation outline`)
updated `README.md` and `research/presentation/outline.md` only.
It was published on `main` after `v4.6.0` (R4.7.15).
It did not alter the frozen package inventory files or the manifest self-hash.

## 8. Current main versus immutable tag

Verified baseline for this clarification:

| Ref | Commit | Role |
|-----|--------|------|
| `v4.6.0` (peel) | `0b7feadc8d77f56c293c04a52e8fb434580f134f` | Frozen research publication package tip |
| `main` (verified baseline) | `25b813acaade94271b327168577e74261849ddbe` | Same history + public navigation/outline alignment |

**Intentional:** current `main` and immutable `v4.6.0` point to **different** commits.
Package inventory files are unchanged between those two commits.

## 9. GitHub Release object distinction

An annotated Git tag is **not** a GitHub Release.

At the verified baseline for this document, a read-only GitHub API listing reported **zero** repository Release objects
(including no Release for `v4.6.0`).
Do not infer a Release UI entry from the presence of the tag alone.

## 10. What this document does not change

This clarification does **not**:

- Modify release notes, the release manifest, or other frozen package members.
- Rewrite historical SHA values or provenance fields.
- Recalculate or rewrite `manifest_sha256` / `manifest_bytes`.
- Move, recreate, or delete tag `v4.6.0`.
- Create a GitHub Release.
- Claim third-party reproduction or new technical validation results.
- Add this file to the frozen R4.6 package inventory.
