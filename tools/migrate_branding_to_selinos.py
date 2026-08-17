#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Migrate authored SeLinOS branding from legacy Helix names.

The repository directory name is deliberately left intact during this migration;
it is a workspace path, not a shipped product identifier. Vendored seL4 sources,
build outputs and binary fixtures are excluded.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TARGETS = [
    ROOT / "README.md",
    ROOT / "docs",
    ROOT / "tests/artifacts/selinos_driver_runtime_m1.verification.json",
    ROOT / "tools/verify_driver_runtime_m1.py",
    ROOT / "src/projects/helixos/CMakeLists.txt",
    ROOT / "src/projects/helixos/include",
    ROOT / "src/projects/helixos/src",
    ROOT / "src/projects/helixos/servers",
    ROOT / "src/projects/helixos/drivers",
]
SUFFIXES = {".md", ".c", ".h", ".txt", ".json", ".py", ".cmake"}
REPLACEMENTS = (
    ("HelixOS", "SeLinOS"),
    ("HELIX_", "SELINOS_"),
    ("Helix", "SeLin"),
    ("helix", "selinos"),
)


def eligible(path: Path) -> bool:
    return path.is_file() and path.suffix in SUFFIXES


def migrate_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    updated = text
    for old, new in REPLACEMENTS:
        updated = updated.replace(old, new)
    # Legacy `helixos` consists of the Helix stem plus `os`; a stem-level
    # replacement therefore yields `selinosos`. The canonical target stem is
    # `selinos`, while the public product spelling remains SeLinOS.
    updated = updated.replace("selinosos", "selinos")
    if updated == text:
        return False
    path.write_text(updated, encoding="utf-8")
    return True


def main() -> int:
    changed: list[Path] = []
    for target in TARGETS:
        candidates = [target] if target.is_file() else sorted(target.rglob("*"))
        for path in candidates:
            if eligible(path) and migrate_file(path):
                changed.append(path.relative_to(ROOT))

    legacy_header = ROOT / "src/projects/helixos/include/helix_bootstrap.h"
    selinos_header = ROOT / "src/projects/helixos/include/selinos_bootstrap.h"
    if legacy_header.exists():
        legacy_header.rename(selinos_header)
        changed.append(selinos_header.relative_to(ROOT))

    for path in changed:
        print(path)
    print(f"migrated_files={len(changed)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
