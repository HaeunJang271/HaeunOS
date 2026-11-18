#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
COMMIT="$(git rev-parse HEAD)"
DIRTY="$(git status --porcelain | wc -l | tr -d ' ')"
echo "commit=$COMMIT dirty≈$DIRTY"
make clean && make all && make test
# manifest
{
  echo "{"
  echo "  \"commit\": \"$COMMIT\","
  echo "  \"dirty_approx\": $DIRTY,"
  echo "  \"third_party\": false"
  echo "}"
} > research/reproduction/r43-artifact-manifest.json
echo "r43 local package run OK"
