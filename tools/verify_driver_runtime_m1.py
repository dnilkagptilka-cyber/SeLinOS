#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the committed SeLinOS Driver-runtime M1 evidence record."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record_path = project / "tests/artifacts/selinos_driver_runtime_m1.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))

    image = record["image"]
    image_path = project / image["path"]
    require(image_path.is_file(), f"missing image: {image_path}")
    require(sha256_file(image_path) == image["sha256"], "image SHA-256 mismatch")

    runtime = record["runtime_evidence"]
    for label, evidence in runtime.items():
        evidence_path = project / evidence["path"]
        require(evidence_path.is_file(), f"missing {label} evidence: {evidence_path}")
        require(sha256_file(evidence_path) == evidence["sha256"],
                f"{label} SHA-256 mismatch")
        content = evidence_path.read_text(encoding="utf-8", errors="replace")
        for marker in evidence["required_markers"]:
            require(marker in content, f"missing {label} marker: {marker}")

    kabi = record["kabi_regression"]
    kabi_path = project / kabi["report_path"]
    require(kabi_path.is_file(), f"missing KABI report: {kabi_path}")
    require(sha256_file(kabi_path) == kabi["report_sha256"], "KABI report SHA-256 mismatch")
    kabi_report = json.loads(kabi_path.read_text(encoding="utf-8"))
    require(kabi_report.get("accepted_for_next_loader_stage") is True,
            "KABI report is not accepted for next loader stage")
    mismatches = kabi_report.get("version_mismatches")
    require(mismatches == 0 or mismatches == {},
            "KABI report has version mismatches")

    print("SeLinOS Driver-runtime M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
