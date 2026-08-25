#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify Phase 91 reproducible SeLinOS x86 execute-disable dependencies."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


EXPECTED_SOURCES = {
    "kernel/seL4": "c7b5e55ec8cdddb506d69d96fdc69dc981088571",
    "projects/seL4_libs": "37b55704c1480ca6a8234cf962d01c099d20e7a1",
    "projects/musllibc": "b0005f86fecbd6d0257b15363a5b013446914265",
    "projects/sel4runtime": "86489cf6efab9f314964e79468c036e9035394c7",
    "projects/util_libs": "6e55b3c62687779692150e1de411ce61b9d2919a",
    "projects/sel4_projects_libs": "5b3c81127b191232489df09a59b22edead1c9db7",
    "tools/seL4_tools": "7dd5ba144b1fecf1358a12d2bef3eb365aab35c7",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def bind(project: Path, binding: dict[str, str], label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def parse_lock(lock_text: str) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line in lock_text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        revision, name = stripped.split(maxsplit=1)
        require(len(revision) == 40, f"invalid source revision length for {name}")
        require(name not in entries, f"duplicate lock entry: {name}")
        entries[name] = revision
    return entries


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (project / "tests/artifacts/phase91_reproducible_nx_dependencies.verification.json").read_text(
            encoding="utf-8"
        )
    )
    require(evidence["schema"] == 1, "unexpected Phase 91 evidence schema")
    require(
        evidence["scope"] == "reproducible x86_64 execute-disable dependency pinning",
        "unexpected Phase 91 scope",
    )

    bootstrap = bind(project, evidence["implementation"]["bootstrap"], "bootstrap source")
    lockfile = bind(project, evidence["implementation"]["lockfile"], "source lock")
    bootstrap_text = bootstrap.read_text(encoding="utf-8")
    lock_text = lockfile.read_text(encoding="utf-8")

    required_bootstrap_fragments = (
        "https://github.com/dnilkagptilka-cyber/seL4.git",
        EXPECTED_SOURCES["kernel/seL4"],
        "https://github.com/dnilkagptilka-cyber/seL4_libs.git",
        EXPECTED_SOURCES["projects/seL4_libs"],
        "x86_64 execute-disable API",
        "SeLinOS seL4 and seL4_libs forks are pinned",
    )
    for fragment in required_bootstrap_fragments:
        require(fragment in bootstrap_text, f"bootstrap lacks required dependency pin: {fragment}")

    require(
        'fetch "kernel/seL4" "https://github.com/seL4/seL4.git"' not in bootstrap_text,
        "bootstrap still selects upstream seL4 instead of the reviewed SeLinOS fork",
    )
    require(
        'fetch "projects/seL4_libs" "https://github.com/seL4/seL4_libs.git"' not in bootstrap_text,
        "bootstrap still selects upstream seL4_libs instead of the reviewed SeLinOS fork",
    )

    expected_bootstrap_order = (
        'fetch "kernel/seL4" "https://github.com/dnilkagptilka-cyber/seL4.git"',
        'fetch "projects/seL4_libs" "https://github.com/dnilkagptilka-cyber/seL4_libs.git"',
    )
    positions = [bootstrap_text.index(fragment) for fragment in expected_bootstrap_order]
    require(positions == sorted(positions), "fork dependency bootstrap order changed unexpectedly")

    require(
        "# SeLinOS seL4 and seL4_libs forks are pinned for the x86_64 execute-disable API"
        in lock_text,
        "source lock does not declare the forked x86 execute-disable interface",
    )
    actual_sources = parse_lock(lock_text)
    require(actual_sources == EXPECTED_SOURCES, "source lock does not match the complete expected source set")

    print("SeLinOS Phase 91 reproducible x86 execute-disable dependencies verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
