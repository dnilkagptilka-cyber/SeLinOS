#!/usr/bin/env bash
# Rebuild every isolated SeLinOS profile after Phase 78 M3 NXE changes.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LOG="$ROOT/tests/artifacts/phase78_m3_all_profile_rebuild.log"

: > "$LOG"
count=0
while IFS= read -r build_dir; do
    name=$(basename "$build_dir")
    printf 'BEGIN %s\n' "$name" | tee -a "$LOG"
    cmake --build "$build_dir" -- -j2 2>&1 | tee -a "$LOG"
    printf 'PASS %s\n' "$name" | tee -a "$LOG"
    count=$((count + 1))
done < <(find "$ROOT" -maxdepth 1 -mindepth 1 -type d -name 'build-*' -printf '%p\n' | LC_ALL=C sort)
printf 'ALL_PROFILE_REBUILD_PASS count=%s\n' "$count" | tee -a "$LOG"
