#!/usr/bin/env python3
"""Read-only audit of evidence hashes affected by the NXE correction."""
from __future__ import annotations

import hashlib
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bindings(value: object, prefix: str = "") -> list[tuple[str, dict[str, object]]]:
    if isinstance(value, dict):
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            return [(prefix or "binding", value)]
        found: list[tuple[str, dict[str, object]]] = []
        for key, item in value.items():
            found.extend(bindings(item, f"{prefix}.{key}" if prefix else key))
        return found
    if isinstance(value, list):
        found = []
        for index, item in enumerate(value):
            found.extend(bindings(item, f"{prefix}[{index}]"))
        return found
    return []


def main() -> None:
    manifests = sorted(ARTIFACTS.glob("*.verification.json"))
    affected: list[tuple[str, str, str]] = []
    categories: Counter[str] = Counter()
    total = 0
    for manifest in manifests:
        try:
            data = json.loads(manifest.read_text())
        except json.JSONDecodeError:
            continue
        for section in ("images", "implementation"):
            for label, binding in bindings(data.get(section, {}), section):
                path = ROOT / str(binding["path"])
                total += 1
                if not path.is_file() or sha256(path) != binding["sha256"]:
                    categories[section] += 1
                    affected.append((manifest.name, label, str(binding["path"])))
    print(f"MANIFESTS={len(manifests)}")
    print(f"BOUND_INPUTS={total}")
    print(f"MISMATCHES={len(affected)}")
    for category, count in sorted(categories.items()):
        print(f"MISMATCH_{category.upper()}={count}")
    for manifest, label, path in affected:
        print(f"STALE {manifest}\t{label}\t{path}")


if __name__ == "__main__":
    main()
