#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS bounded dual-QEMU-edu M3 evidence."""
import hashlib
import json
import sys
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    record_path = root / "tests/artifacts/selinos_m3_two_edu.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))

    image = root / record["image"]["path"]
    require(image.is_file(), f"missing image: {image}")
    require(sha256(image) == record["image"]["sha256"], "image SHA-256 mismatch")

    runtime = record["runtime_evidence"]
    log = root / runtime["path"]
    require(log.is_file(), f"missing M3 runtime log: {log}")
    require(sha256(log) == runtime["sha256"], "M3 runtime log SHA-256 mismatch")
    text = log.read_text(encoding="utf-8", errors="replace")

    for marker in runtime["required_once"]:
        require(text.count(marker) >= 1, f"missing M3 marker: {marker}")
    for marker in runtime["required_twice"]:
        require(text.count(marker) >= 2, f"expected two M3 lifecycle occurrences: {marker}")

    kabi = record["kabi_regression"]
    report = root / kabi["report_path"]
    require(report.is_file(), f"missing KABI report: {report}")
    require(sha256(report) == kabi["report_sha256"], "KABI report SHA-256 mismatch")
    report_data = json.loads(report.read_text(encoding="utf-8"))
    require(report_data.get("accepted_for_next_loader_stage") is True,
            "KABI report is not accepted for next loader stage")
    mismatches = report_data.get("version_mismatches")
    require(isinstance(mismatches, dict) and len(mismatches) == kabi["version_mismatches"],
            "KABI version mismatch count differs from M3 record")

    print("SeLinOS M3 dual-QEMU-edu evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, RuntimeError) as error:
        print(f"M3 evidence verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
