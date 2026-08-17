#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS development-only pinned-fixture execution evidence."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RECORD = ROOT / "tests/artifacts/selinos_kabi_fixture_execution.verification.json"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    fixture = ROOT / record["fixture"]["path"]
    kabi_report = ROOT / record["kabi_report"]["path"]
    require(sha256(fixture) == record["fixture"]["sha256"], "fixture hash mismatch")
    require(sha256(kabi_report) == record["kabi_report"]["sha256"], "KABI report hash mismatch")
    lifecycle = record["entrypoint_lifecycle"]
    isolation = record["isolation_scope"]
    require(lifecycle["module_code_executed"] is True, "execution evidence must report code execution")
    require(lifecycle["init_module_return"] == 0, "init_module success is required")
    require(lifecycle["cleanup_module_called"] is True, "cleanup_module call is required")
    require(lifecycle["curated_printk_calls"] == 2, "expected two curated printk calls")
    require(isolation["dedicated_sel4_domain"] is True, "dedicated domain evidence is required")
    require(isolation["device_capabilities_granted"] is False, "fixture execution must have no device caps")
    require(isolation["nx_enforced_wx"] is False, "record must not claim NX-enforced W^X")
    binary = ROOT / "tests/kabi_module_execution_test"
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fno-pie", "-no-pie",
        "-Isrc/projects/helixos/kabi/include",
        "src/projects/helixos/kabi/src/selinos_kabi_module.c",
        "src/projects/helixos/kabi/src/selinos_kabi_policy.c",
        "src/projects/helixos/kabi/src/selinos_kabi_exports.c",
        "tests/kabi_module_execution_test.c", "-o", str(binary.relative_to(ROOT)),
    ]
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(binary), str(fixture)], cwd=ROOT, check=True,
                            text=True, capture_output=True)
    require("SeLinOS KABI controlled fixture execution regression passed." in result.stdout,
            "missing controlled execution success marker")
    require(result.stdout.count("host fixture _printk stub invoked") == 2,
            "expected exactly two curated printk calls")
    print("SeLinOS KABI controlled fixture-execution evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"fixture-execution evidence verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
