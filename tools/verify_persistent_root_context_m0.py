#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def bind(root: Path, item: dict, label: str) -> Path:
    path = root / item["path"]
    need(path.is_file(), f"missing {label}")
    need(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / "tests/artifacts/selinos_persistent_root_context_m0.verification.json").read_text())
    need(record["schema"] == 1, "invalid persistent root-context evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid persistent root-context platform evidence")
    bind(root, record["image"], "image")
    source = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "static simple_t root_simple_context;",
        "static vka_t root_vka_context;",
        "static vspace_t root_vspace_context;",
        "static bool root_construction_context_initialized;",
        "simple_t *const simple = &root_simple_context;",
        "vka_t *const vka = &root_vka_context;",
        "vspace_t *const vspace = &root_vspace_context;",
        "if (root_construction_context_initialized)",
        "root_construction_context_initialized = true;",
    ):
        need(required in source, f"missing persistent-context control: {required}")

    for forbidden in (
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST",
        "SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED",
        "start_root_construction_adapter",
        "root construction M1: child constructed",
    ):
        need(forbidden not in source, f"unexpected construction implementation: {forbidden}")
    dispatch = source[source.index("bool selinos_root_dispatch_m0_once(void)"):
                      source.index("/* Phase 33")]
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_TCB_",
        "seL4_Untyped_Retype", "SELINOS_LINUX_CLONE",
    ):
        need(forbidden not in dispatch, f"root dispatch unexpectedly expanded authority: {forbidden}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")

    claims = " ".join(record["not_claimed"])
    for excluded in ("runtime child construction", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS persistent root construction context M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
