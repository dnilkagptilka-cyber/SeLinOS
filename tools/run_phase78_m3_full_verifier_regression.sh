#!/usr/bin/env bash
# Run every standalone verifier after Phase 78 M3 evidence creation.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LOG="$ROOT/tests/artifacts/phase78_m3_full_verifier_regression.log"

: > "$LOG"
count=0
while IFS= read -r verifier; do
    name=$(basename "$verifier")
    case "$name" in
        verify_kabi_module.py|verify_m15_probe_object_window.py)
            printf 'SKIP %s requires dedicated positional inputs\n' "$name" | tee -a "$LOG"
            continue
            ;;
    esac
    printf 'BEGIN %s\n' "$name" | tee -a "$LOG"
    python3 "$verifier" </dev/null 2>&1 | tee -a "$LOG"
    printf 'PASS %s\n' "$name" | tee -a "$LOG"
    count=$((count + 1))
done < <(find "$ROOT/tools" -maxdepth 1 -type f -name 'verify_*.py' -printf '%p\n' | LC_ALL=C sort)
printf 'FULL_STANDALONE_VERIFIER_REGRESSION_PASS count=%s\n' "$count" | tee -a "$LOG"
