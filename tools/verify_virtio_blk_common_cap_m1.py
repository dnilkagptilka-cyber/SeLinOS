#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only modern virtio common-capability M1 evidence."""
import hashlib
import json
import sys
from pathlib import Path


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def verify_bound(project, item, label):
    path = project / item["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main():
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads((project / "tests/artifacts/selinos_virtio_blk_common_cap_m1.verification.json").read_text())
    verify_bound(project, evidence["image"], "opt-in image")
    for label, item in evidence["implementation"].items():
        verify_bound(project, item, label)
    require(evidence["profile"]["cmake_option"] == "SeLinRootVirtioBlkCapabilityProbe=ON", "wrong experiment gate")
    require(evidence["profile"]["device_identity"] == "1af4:1042", "wrong device identity")
    runtime = verify_bound(project, evidence["runtime_evidence"], "runtime log")
    text = runtime.read_text(errors="replace")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden marker: {marker}")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("BAR", "feature", "DMA", "block reads or writes", "dpkg"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")
    print("SeLinOS virtio-blk common-capability M1 evidence verified.")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
