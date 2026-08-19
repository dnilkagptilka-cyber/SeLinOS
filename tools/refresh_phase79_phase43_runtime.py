#!/usr/bin/env python3
"""Refresh only two independently replayed Phase 43 runtime log hashes."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFESTS = (
    "selinos_x86_nx_mapping_m0.verification.json",
    "selinos_x86_nx_raw_fsr_revalidation_m1.verification.json",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    for name in MANIFESTS:
        path = ROOT / "tests/artifacts" / name
        data = json.loads(path.read_text())
        runtime = data.get("runtime_evidence", data.get("runtime"))
        if not isinstance(runtime, dict):
            raise RuntimeError(f"{name}: missing runtime binding")
        log = ROOT / runtime["path"]
        text = log.read_text(errors="replace")
        for marker in runtime["required_markers"]:
            if marker not in text:
                raise RuntimeError(f"{name}: missing marker {marker!r}")
        for marker in runtime["forbidden_markers"]:
            if marker in text:
                raise RuntimeError(f"{name}: forbidden marker {marker!r}")
        runtime["sha256"] = sha256(log)
        path.write_text(json.dumps(data, indent=2) + "\n")
        print(f"REFRESHED {name}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"refresh failed: {error}")
        raise SystemExit(1)
