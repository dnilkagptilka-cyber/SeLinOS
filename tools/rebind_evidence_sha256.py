#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Refresh only existing file SHA-256 bindings in SeLinOS evidence records."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


PROJECT = Path(__file__).resolve().parent.parent
ARTIFACTS = PROJECT / "tests" / "artifacts"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def refresh_bindings(value: object, changed: list[str]) -> None:
    if isinstance(value, dict):
        path_value = value.get("path")
        digest_value = value.get("sha256")
        if isinstance(path_value, str) and isinstance(digest_value, str):
            path = PROJECT / path_value
            if path.is_file():
                actual = sha256(path)
                if value["sha256"] != actual:
                    value["sha256"] = actual
                    changed.append(path_value)
        for child in value.values():
            refresh_bindings(child, changed)
    elif isinstance(value, list):
        for child in value:
            refresh_bindings(child, changed)


def main() -> int:
    total = 0
    for record_path in sorted(ARTIFACTS.glob("*.verification.json")):
        record = json.loads(record_path.read_text(encoding="utf-8"))
        changed: list[str] = []
        refresh_bindings(record, changed)
        if changed:
            record_path.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
            print(f"{record_path.relative_to(PROJECT)}: refreshed {len(changed)} binding(s)")
            total += len(changed)
    print(f"Refreshed {total} SHA-256 evidence binding(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
