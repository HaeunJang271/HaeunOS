#!/usr/bin/env bash
# POSIX counterpart: separate-directory copy clean build (same limitations as .ps1).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
STAMP="$(date +%Y%m%d-%H%M%S)"
WORK="$(dirname "$ROOT")/HaeunOS-r42-repro-${STAMP}"
OUT="$ROOT/research/reproduction"
mkdir -p "$WORK" "$OUT" "$ROOT/research/experiments"
rsync -a --exclude build --exclude .git "$ROOT/" "$WORK/"
COMMIT="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
{
  echo "{"
  echo "  \"reproduction_kind\": \"separate_directory_working_tree_copy\","
  echo "  \"third_party_independent\": false,"
  echo "  \"git_commit_at_copy\": \"$COMMIT\","
  echo "  \"work_dir\": \"$WORK\""
  echo "}"
} >"$OUT/r42-environment.json"
cd "$WORK"
make clean && make all
make build/r12-images.stamp build/r22-images.stamp build/r30-images.stamp
make build/r42-validation-tests
mkdir -p research/experiments
./build/r42-validation-tests build | tee "$OUT/r42-reproduction-run.log"
cp research/experiments/r42-*.json "$ROOT/research/experiments/"
sha256sum kernel/process/proc_*.s research/experiments/r42-*.json >"$OUT/r42-manifest.sha256.txt" || true
echo "{\"git_commit_at_copy\":\"$COMMIT\",\"work_dir\":\"$WORK\",\"note\":\"see sha256 txt\"}" >"$OUT/r42-manifest.json"
