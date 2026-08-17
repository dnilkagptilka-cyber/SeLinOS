#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the SeLinOS Linux syscall ABI M5 identity/TID evidence."""

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


def bound_file(project: Path, binding: dict[str, str], label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record = json.loads((project / "tests/artifacts/selinos_linux_syscall_abi_m5.verification.json").read_text(
        encoding="utf-8"
    ))

    bound_file(project, record["image"], "image")
    for label, binding in record["implementation"].items():
        bound_file(project, binding, label)

    expected_contract = [
        (102, "getuid", 0),
        (104, "getgid", 0),
        (107, "geteuid", 0),
        (108, "getegid", 0),
        (186, "gettid", 4243),
        (218, "set_tid_address", 4243),
    ]
    observed_contract = [
        (entry["number"], entry["name"], entry["result"])
        for entry in record["syscall_contract"]
    ]
    require(observed_contract == expected_contract, "M5 syscall contract changed")
    require("neither retained nor written" in record["syscall_contract"][-1]["input_policy"],
            "set_tid_address persistence contract broadened")

    runtime = bound_file(project, record["runtime_evidence"], "runtime evidence")
    text = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in text, f"missing runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in text, f"forbidden runtime marker: {marker}")

    exclusions = " ".join(record["not_claimed"])
    require("arch_prctl" in exclusions and "clone" in exclusions and "credentials" in exclusions,
            "M5 exclusions incomplete")
    print("SeLinOS Linux syscall ABI M5 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
