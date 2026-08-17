#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Linux-to-volatile-VFS bridge M7 evidence."""

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
    record_path = project / "tests/artifacts/selinos_linux_vfs_volatile_bridge_m7.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M7 must remain a native seL4 proof")
    verify_binding(project, record["image"], "M7 image")
    for label, binding in record["implementation"].items():
        verify_binding(project, binding, label)
    log = verify_binding(project, record["runtime_evidence"], "M7 runtime log")
    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M7 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M7 marker: {marker}")

    contract = record["contract"]
    for key, phrase in (
        ("invalid_open", "-ENOENT before any server OPEN_PATH IPC"),
        ("open", "exact NUL-terminated `/selinos-state`"),
        ("read", "exactly eight bytes"),
        ("write", "fixed eight-byte `STATEM0!`"),
        ("lifecycle", "-EBADF"),
    ):
        require(phrase in contract[key], f"M7 contract incomplete: {key}")

    exclusions = " ".join(record["not_claimed"])
    for phrase in ("persistent data", "DMA", "arbitrary pathnames", "POSIX", "dpkg or apt"):
        require(phrase in exclusions, f"missing M7 non-claim: {phrase}")

    print("SeLinOS Linux-to-volatile-VFS bridge M7 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
