#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the SeLinOS Phase 6 M1 bounded KAPI synchronization evidence."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def verify_bound_file(project: Path, binding: dict[str, str], label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record_path = project / "tests/artifacts/selinos_kapi_sync_m1.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))

    verify_bound_file(project, record["image"], "image")
    for label, binding in record["implementation"].items():
        verify_bound_file(project, binding, label)

    runtime = verify_bound_file(project, record["runtime_evidence"], "runtime evidence")
    text = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden runtime marker: {marker}")

    bounded = record["bounded_contract"]
    require("non-blocking" in bounded["completion"], "completion contract broadened")
    require("synchronously" in bounded["workqueue"], "workqueue contract broadened")
    require("no clock service" in bounded["timer"], "timer contract broadened")
    require("does not mask IRQs" in bounded["spinlock"], "spinlock contract broadened")

    print("SeLinOS KAPI synchronization M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
