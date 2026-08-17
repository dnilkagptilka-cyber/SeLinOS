#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS Linux x86_64 syscall ABI M1 evidence."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RECORD = ROOT / "tests/artifacts/selinos_linux_syscall_abi_m1.verification.json"
LOG = ROOT / "tests/artifacts/selinos_linux_syscall_abi_m1.boot.log"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    source = ROOT / record["probe_source"]["path"]
    manager = ROOT / record["gateway_source"]["path"]
    require(sha256_bytes(source.read_bytes()) == record["probe_source"]["sha256"],
            "syscall probe source hash mismatch")
    require(sha256_bytes(manager.read_bytes()) == record["gateway_source"]["sha256"],
            "domain manager source hash mismatch")
    require("syscall" in source.read_text(encoding="utf-8"),
            "probe must contain the native x86_64 syscall instruction")
    gateway_source = manager.read_text(encoding="utf-8")
    require("seL4_Fault_UnknownSyscall" in gateway_source,
            "gateway must gate UnknownSyscall faults")
    require("seL4_UnknownSyscall_RAX" in gateway_source,
            "gateway must authenticate the Linux syscall number in RAX")
    require("seL4_UnknownSyscall_FaultIP" in gateway_source,
            "gateway must advance the faulting syscall instruction")
    require("seL4_TCB_WriteRegisters" not in gateway_source,
            "M1 reply path must not rewrite unrelated TCB state")

    subprocess.run(["ninja"], cwd=BUILD, check=True)
    command = (
        "timeout 12s qemu-system-x86_64 -cpu max -nographic -serial mon:stdio "
        "-m size=1G -kernel images/kernel-x86_64-pc99 "
        "-initrd images/selinos-root-image-x86_64-pc99"
    )
    result = subprocess.run(["script", "-qefc", command, str(LOG)], cwd=BUILD)
    output = LOG.read_text(encoding="utf-8", errors="replace")
    require(result.returncode == 124,
            f"QEMU must remain live until timeout, got {result.returncode}")
    required_markers = record["required_serial_markers"]
    for marker in required_markers:
        require(marker in output, f"missing serial marker: {marker}")
    for marker in record["forbidden_serial_markers"]:
        require(marker not in output, f"forbidden serial marker: {marker}")
    require(output.index(required_markers[0]) < output.index(required_markers[1]),
            "gateway acknowledgement must precede normal boot completion")
    print("SeLinOS Linux syscall ABI M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"Linux syscall ABI M1 verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
