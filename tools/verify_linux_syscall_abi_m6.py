#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Linux syscall ABI M6 evidence."""

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
    record = json.loads((project / "tests/artifacts/selinos_linux_syscall_abi_m6.verification.json")
                        .read_text(encoding="utf-8"))
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M6 must remain a native seL4 ABI proof")
    verify_binding(project, record["image"], "M6 image")
    for label, binding in record["implementation"].items():
        verify_binding(project, binding, label)
    log = verify_binding(project, record["runtime_evidence"], "M6 runtime log")
    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M6 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M6 marker: {marker}")

    contract = record["contract"]
    require("CLOCK_MONOTONIC (1)" in contract["clock_gettime"],
            "clock_gettime must remain fixed to CLOCK_MONOTONIC")
    require("{1234, 0}" in contract["clock_gettime"],
            "clock_gettime result must remain deterministic")
    require("RLIMIT_NOFILE (7)" in contract["getrlimit"],
            "getrlimit must remain fixed to RLIMIT_NOFILE")
    require("{64, 64}" in contract["getrlimit"],
            "getrlimit values must remain fixed")
    require("-EAGAIN immediately" in contract["futex_wait"],
            "futex wait must remain non-blocking")
    require("creates no waiter or scheduler state" in contract["futex_wake"],
            "futex wake must not create scheduler state")
    require("EAX" in contract["reply_encoding"],
            "negative result encoding must be documented")

    exclusions = " ".join(record["not_claimed"])
    for phrase in ("blocking futex", "TLS", "ARCH_SET_FS", "clone", "dpkg or apt"):
        require(phrase in exclusions, f"missing M6 non-claim: {phrase}")

    print("SeLinOS Linux syscall ABI M6 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
