#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS non-executing Linux ET_REL relocation-stage evidence."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def parse_key_values(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    evidence = json.loads((root / "tests/artifacts/selinos_kabi_relocation_stage.verification.json").read_text())
    module = root / evidence["module"]["path"]
    report = root / evidence["kabi_report"]["path"]
    require(digest(module) == evidence["module"]["sha256"], "module SHA-256 mismatch")
    require(digest(report) == evidence["kabi_report"]["sha256"], "KABI report SHA-256 mismatch")
    report_data = json.loads(report.read_text())
    require(report_data.get("accepted_for_next_loader_stage") is True,
            "KABI report not accepted for loader stage")
    mismatches = report_data.get("version_mismatches")
    require(isinstance(mismatches, dict) and
            len(mismatches) == evidence["kabi_report"]["version_mismatch_count"],
            "unexpected KABI version mismatch count")

    binary = root / "tests/kabi_module_parser_test"
    compile_result = subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-Isrc/projects/helixos/kabi/include",
        "src/projects/helixos/kabi/src/selinos_kabi_module.c",
        "src/projects/helixos/kabi/src/selinos_kabi_policy.c",
        "src/projects/helixos/kabi/src/selinos_kabi_exports.c",
        "tests/kabi_module_parser_test.c", "-o", str(binary),
    ], cwd=root, text=True, capture_output=True)
    require(compile_result.returncode == 0,
            f"relocation regression compile failed: {compile_result.stderr}")
    run_result = subprocess.run([str(binary), str(module)], cwd=root,
                                text=True, capture_output=True)
    require(run_result.returncode == 0,
            f"relocation regression failed: {run_result.stderr}")
    values = parse_key_values(run_result.stdout)
    expected = evidence["expected_output"]
    require(values.get("pinned_digest_guard") == "passed",
            "pinned digest policy guard was not exercised")
    require(values.get("curated_export_guard") == "passed",
            "curated export policy guard was not exercised")
    require(values.get("module_versions_guard") == "passed",
            "runtime module-version CRC guard was not exercised")
    for key in ("name", "relocation_sections", "undefined_symbols",
                "relocated_sections", "applied_relocations"):
        require(values.get(key) == str(expected[key]),
                f"unexpected relocation output {key}: {values.get(key)!r}")
    for key in ("init_module", "cleanup_module"):
        require(values.get(key, "0x0") not in ("0", "0x0"),
                f"missing relocated entry address: {key}")

    print("SeLinOS KABI non-executing relocation-stage evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, RuntimeError) as error:
        print(f"KABI relocation-stage verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
