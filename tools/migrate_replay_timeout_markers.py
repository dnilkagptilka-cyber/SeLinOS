#!/usr/bin/env python3
"""Migrate replayed evidence from a wrapper-specific timeout marker to script's record."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
OLD = "qemu_status=124"
NEW = 'COMMAND_EXIT_CODE="124"'


def runtime_binding(data: dict[str, object]) -> dict[str, object] | None:
    for key in ("runtime", "runtime_evidence"):
        value = data.get(key)
        if isinstance(value, dict) and isinstance(value.get("path"), str):
            return value
    return None


def main() -> None:
    changed: list[str] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        data = json.loads(manifest.read_text())
        runtime = runtime_binding(data)
        if runtime is None:
            continue
        markers = runtime.get("required_markers")
        if not isinstance(markers, list) or OLD not in markers:
            continue
        log = ROOT / str(runtime["path"])
        if not log.is_file() or NEW not in log.read_text(errors="replace"):
            raise SystemExit(f"migration refused: {manifest.name} lacks replay timeout record")
        runtime["required_markers"] = [NEW if marker == OLD else marker for marker in markers]
        manifest.write_text(json.dumps(data, indent=2) + "\n")
        changed.append(manifest.name)
    print(f"REPLAY_TIMEOUT_MARKER_MIGRATION_PASS count={len(changed)}")
    for manifest in changed:
        print(f"MIGRATED {manifest}")


if __name__ == "__main__":
    main()
