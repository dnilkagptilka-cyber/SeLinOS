#!/usr/bin/env python3
"""Refresh only residual comprehensive-audit bindings after NXE replays.

Allowed mismatches are deliberately narrow: replay-backed image outputs, aliases
for domain_manager.c, and the generated config for the already replayed M3
profile.  Any other record is an error.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
REPLAY_LOG = ARTIFACTS / "phase78_nxe_affected_profile_replays.log"
ALLOWED_PATHS = {
    "src/projects/helixos/CMakeLists.txt",
    "src/projects/helixos/src/domain_manager.c",
    "src/projects/helixos/elfrt/src/selinos_elfrt.c",
    "src/projects/helixos/elfrt/include/selinos_elfrt.h",
    "build-fresh-target-nxe-stack-revalidation-probe/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-x86-nx-raw-fsr-revalidation-probe/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-rx-load-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-rx-rw-bss-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-rx-ro-rw-bss-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-two-rx-ro-rw-bss-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-two-rx-gnu-stack-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-static-elf-two-rx-note-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
    "build-controlled-et-dyn-relative-m0/projects/helixos/gen_config/selinos-root/gen_config.h",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def walk(value: object) -> list[dict[str, object]]:
    if isinstance(value, dict):
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            return [value]
        found: list[dict[str, object]] = []
        for item in value.values():
            found.extend(walk(item))
        return found
    if isinstance(value, list):
        found: list[dict[str, object]] = []
        for item in value:
            found.extend(walk(item))
        return found
    return []


def replayed_manifests() -> set[str]:
    text = REPLAY_LOG.read_text()
    if "AFFECTED_PROFILE_REPLAY_PASS" not in text:
        raise SystemExit("refresh refused: no complete replay log")
    result: set[str] = set()
    for line in text.splitlines():
        if line.startswith("BEGIN ") and " manifests=" in line:
            result.update(name for name in line.split(" manifests=", 1)[1].split(",") if name)
    return result


def is_image(relative: str) -> bool:
    parts = Path(relative).parts
    return len(parts) >= 3 and parts[0].startswith("build-") and parts[1] == "images"


def main() -> None:
    replayed = replayed_manifests()
    changed: list[str] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        data = json.loads(manifest.read_text())
        dirty = False
        for binding in walk(data):
            relative = str(binding["path"])
            path = ROOT / relative
            if not path.is_file():
                raise SystemExit(f"refresh refused: missing {relative} in {manifest.name}")
            actual = sha256(path)
            if actual == binding["sha256"]:
                continue
            if is_image(relative):
                if manifest.name not in replayed:
                    raise SystemExit(f"refresh refused: stale image without replay {manifest.name}")
            elif relative.startswith("tests/artifacts/") and relative.endswith(".boot.log"):
                if manifest.name not in replayed:
                    raise SystemExit(f"refresh refused: stale runtime without replay {manifest.name}")
            elif relative not in ALLOWED_PATHS:
                raise SystemExit(f"refresh refused: non-allowlisted stale binding {relative} in {manifest.name}")
            binding["sha256"] = actual
            dirty = True
        if dirty:
            manifest.write_text(json.dumps(data, indent=2) + "\n")
            changed.append(manifest.name)
    print(f"COMPREHENSIVE_EVIDENCE_REFRESH_PASS count={len(changed)}")
    for manifest in changed:
        print(f"REFRESHED {manifest}")


if __name__ == "__main__":
    main()
