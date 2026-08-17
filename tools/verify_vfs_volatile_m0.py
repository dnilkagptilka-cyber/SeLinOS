#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS bounded volatile VFS M0 evidence."""

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


def verify_binding(project: Path, binding: dict[str, str], label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record_path = project / "tests/artifacts/selinos_vfs_volatile_m0.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))

    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "VFS M0 must remain a native seL4 service")
    verify_binding(project, record["image"], "VFS M0 image")
    for label, binding in record["implementation"].items():
        verify_binding(project, binding, label)
    runtime = verify_binding(project, record["runtime_evidence"], "VFS M0 runtime log")
    output = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required VFS M0 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden VFS M0 marker: {marker}")

    contract = record["contract"]
    require("fixed packed pathname /selinos-state" in contract["object"],
            "VFS M0 object must remain fixed")
    require("exactly three IPC words" in contract["write"],
            "VFS M0 write must remain bounded")
    require("only after validation" in contract["write"],
            "VFS M0 must retain atomic validation-before-commit")
    require("later write attempts are rejected" in contract["close"],
            "VFS M0 must retain closed-descriptor rejection")
    require("deliberately volatile" in contract["backing"],
            "VFS M0 must not claim persistence")

    exclusions = " ".join(record["not_claimed"])
    for phrase in ("persistent storage", "block I/O", "DMA", "arbitrary paths",
                   "POSIX filesystem", "dpkg or apt"):
        require(phrase in exclusions, f"missing VFS M0 non-claim: {phrase}")

    print("SeLinOS volatile VFS M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
