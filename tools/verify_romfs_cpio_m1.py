#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the SeLinOS bounded newc CPIO ROMFS M1 evidence."""

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
    record = json.loads((project / "tests/artifacts/selinos_romfs_cpio_m1.verification.json").read_text(
        encoding="utf-8"
    ))

    verify_binding(project, record["image"], "image")
    for label, binding in record["implementation"].items():
        verify_binding(project, binding, label)

    contract = record["archive_contract"]
    require(contract["format"].startswith("uncompressed newc CPIO"), "unexpected archive format")
    require(contract["trailer"] == "TRAILER!!! is embedded and parser-bounded", "trailer contract changed")
    files = contract["files"]
    require(files == [
        {"pathname": "/selinos-release", "content": "SELINOS!", "protocol_fd": 3},
        {"pathname": "/selinos-banner", "content": "CPIO-OK!", "protocol_fd": 4},
    ], "multi-file pathname contract changed")

    runtime = verify_binding(project, record["runtime_evidence"], "runtime evidence")
    text = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden runtime marker: {marker}")

    print("SeLinOS ROMFS CPIO M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
