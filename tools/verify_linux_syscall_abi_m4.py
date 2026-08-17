#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS restricted Linux syscall ABI M4 evidence."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RECORD = ROOT / "tests/artifacts/selinos_linux_syscall_abi_m4.verification.json"
LOG = ROOT / "tests/artifacts/selinos_linux_syscall_abi_m4.boot.log"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    image = ROOT / record["image"]["path"]
    probe = ROOT / record["probe_source"]["path"]
    gateway = ROOT / record["gateway_source"]["path"]

    subprocess.run(["ninja"], cwd=BUILD, check=True)
    require(sha256(image) == record["image"]["sha256"], "image hash mismatch")
    require(sha256(probe) == record["probe_source"]["sha256"], "probe source hash mismatch")
    require(sha256(gateway) == record["gateway_source"]["sha256"], "gateway source hash mismatch")

    probe_source = probe.read_text(encoding="utf-8")
    gateway_source = gateway.read_text(encoding="utf-8")
    for token in ("SELINOS_LINUX_SYS_BRK", "movb $0x6b, -1(%%rax)",
                  "add %[page_size], %%rax"):
        require(token in probe_source, f"probe missing M4 brk token: {token}")
    for token in ("SELINOS_LINUX_BRK 12u", "SELINOS_LINUX_BRK_BASE 0x71000000u",
                  "restricted brk query rejected", "restricted brk grow rejected",
                  "provision_linux_anonymous_page"):
        require(token in gateway_source, f"gateway missing M4 brk token: {token}")
    require("SELINOS_ABI_MAX_WRITE_BYTES 127u" in gateway_source,
            "M4 must retain bounded write limit")

    command = (
        "timeout 12s qemu-system-x86_64 -cpu max -nographic -serial mon:stdio "
        "-m size=1G -kernel images/kernel-x86_64-pc99 "
        "-initrd images/selinos-root-image-x86_64-pc99"
    )
    result = subprocess.run(["script", "-qefc", command, str(LOG)], cwd=BUILD)
    require(result.returncode == 124,
            f"QEMU must remain live until timeout, got {result.returncode}")
    output = LOG.read_text(encoding="utf-8", errors="replace")
    markers = record["required_serial_markers"]
    for marker in markers:
        require(marker in output, f"missing serial marker: {marker}")
    for marker in record["forbidden_serial_markers"]:
        require(marker not in output, f"forbidden serial marker: {marker}")
    require(output.index(markers[0]) < output.index(markers[1]) < output.index(markers[2]),
            "write payload, M4 completion and normal bootstrap must be ordered")
    print("SeLinOS Linux syscall ABI M4 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"Linux syscall ABI M4 verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
