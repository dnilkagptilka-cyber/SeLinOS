#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS ELF runtime parser M1 evidence."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RECORD = ROOT / "tests/artifacts/selinos_elfrt_m1.verification.json"
ARTIFACT_DIR = ROOT / "tests/artifacts/elfrt_m1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    subprocess.run(["ninja", "selinos-elfrt"], cwd=BUILD, check=True)
    for label, source in record["sources"].items():
        path = ROOT / source["path"]
        require(path.is_file(), f"missing {label}")
        require(sha256(path) == source["sha256"], f"{label} hash mismatch")

    ARTIFACT_DIR.mkdir(parents=True, exist_ok=True)
    fixture = ARTIFACT_DIR / "libselinos_elfrt_fixture.so"
    interpreter_fixture = ARTIFACT_DIR / "selinos_elfrt_interp_fixture"
    test = ARTIFACT_DIR / "elfrt_m1_test"
    subprocess.run(["gcc", "-shared", "-fPIC", "-Wl,-z,relro", "-o", str(fixture),
                    str(ROOT / "tests/elfrt_fixture.c")], check=True)
    subprocess.run(["gcc", "-fPIE", "-pie", "-Wl,-z,relro", "-o", str(interpreter_fixture),
                    str(ROOT / "tests/elfrt_interp_fixture.c")], check=True)
    subprocess.run(["gcc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "src/projects/helixos/elfrt/include"),
                    str(ROOT / "tests/elfrt_m1_test.c"),
                    str(ROOT / "src/projects/helixos/elfrt/src/selinos_elfrt.c"),
                    "-o", str(test)], check=True)
    for label, artifact in record["artifacts"].items():
        path = ROOT / artifact["path"]
        require(sha256(path) == artifact["sha256"], f"{label} artifact hash mismatch")
    completed = subprocess.run([str(test), str(fixture), str(interpreter_fixture)], text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    require(completed.returncode == 0, completed.stderr or "ELFRT test failed")
    require(record["required_output"] in completed.stdout, "missing ELFRT test marker")
    print("SeLinOS ELFRT M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"SeLinOS ELFRT M1 verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
