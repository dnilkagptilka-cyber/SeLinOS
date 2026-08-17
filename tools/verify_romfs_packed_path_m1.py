#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS bounded packed-path ROMFS M1 evidence."""
import hashlib
import json
import sys
from pathlib import Path


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def bound(project, item, label):
    path = project / item["path"]
    require(path.is_file(), f"missing {label}")
    require(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main():
    project = Path(__file__).resolve().parent.parent
    record = json.loads((project / "tests/artifacts/selinos_romfs_packed_path_m1.verification.json").read_text())
    bound(project, record["image"], "image")
    for label, item in record["implementation"].items():
        bound(project, item, label)
    contract = record["contract"]
    require(contract["message_words"] == 4, "unexpected IPC width")
    require(contract["accepted_length_range"] == [1, 16], "unexpected path bound")
    require(contract["accepted_runtime_path"] == "/selinos-banner", "unexpected verified path")
    require("user pointers" in contract["rejected_by_design"], "pointer exclusion absent")
    runtime = bound(project, record["runtime_evidence"], "runtime evidence")
    text = runtime.read_text(errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden marker: {marker}")
    print("SeLinOS ROMFS packed-path M1 evidence verified.")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
