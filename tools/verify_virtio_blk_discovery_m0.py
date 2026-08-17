#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS quarantine-only virtio-blk discovery M0 evidence."""
import hashlib
import json
import sys
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def bound(project, item, label):
    path = project / item["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main():
    project = Path(__file__).resolve().parent.parent
    record = json.loads((project / "tests/artifacts/selinos_virtio_blk_discovery_m0.verification.json").read_text())
    bound(project, record["image"], "opt-in image")
    for label, item in record["implementation"].items():
        bound(project, item, label)
    require(record["profile"]["cmake_option"] == "SeLinRootVirtioBlkDiscoveryProbe=ON", "wrong opt-in gate")
    require(record["identity"] == {"vendor_id": "1af4", "device_id": "1042", "kind": "modern QEMU virtio block"},
            "unexpected virtio identity")
    runtime = bound(project, record["runtime_evidence"], "runtime evidence")
    text = runtime.read_text(errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("BAR", "DMA", "block read or write", "dpkg"):
        require(phrase in exclusions, f"missing exclusion: {phrase}")
    print("SeLinOS virtio-blk discovery M0 evidence verified.")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
