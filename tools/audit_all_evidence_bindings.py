#!/usr/bin/env python3
"""Read-only comprehensive SHA binding audit for all evidence-manifest fields."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def walk(value: object, prefix: str = "") -> list[tuple[str, dict[str, object]]]:
    if isinstance(value, dict):
        found: list[tuple[str, dict[str, object]]] = []
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            found.append((prefix or "binding", value))
        for key, item in value.items():
            found.extend(walk(item, f"{prefix}.{key}" if prefix else key))
        return found
    if isinstance(value, list):
        found = []
        for index, item in enumerate(value):
            found.extend(walk(item, f"{prefix}[{index}]"))
        return found
    return []


def main() -> None:
    count = 0
    mismatches: list[tuple[str, str, str]] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        try:
            data = json.loads(manifest.read_text())
        except json.JSONDecodeError:
            continue
        for label, binding in walk(data):
            count += 1
            path = ROOT / str(binding["path"])
            if not path.is_file() or sha256(path) != binding["sha256"]:
                mismatches.append((manifest.name, label, str(binding["path"])))
    print(f"ALL_BINDINGS={count}")
    print(f"ALL_BINDING_MISMATCHES={len(mismatches)}")
    for manifest, label, path in mismatches:
        print(f"STALE {manifest}\t{label}\t{path}")


if __name__ == "__main__":
    main()
