#!/usr/bin/env python3
"""Controlled SHA refresh after global NXE and Phase 43 evidence revalidation.

Only the allowlisted globally changed sources and images/runtime records from the
recorded replay set may change.  Any other mismatch stops the operation.
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
REPLAY_LOG = ARTIFACTS / "phase78_nxe_affected_profile_replays.log"
ALLOWLIST = {
    "src/projects/helixos/CMakeLists.txt",
    "src/projects/helixos/src/domain_manager.c",
    "src/projects/helixos/src/x86_nx_mapping_probe.c",
    "src/projects/helixos/include/selinos_x86_nx_probe_protocol.h",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bindings(value: object) -> list[dict[str, object]]:
    if isinstance(value, dict):
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            return [value]
        found: list[dict[str, object]] = []
        for item in value.values():
            found.extend(bindings(item))
        return found
    if isinstance(value, list):
        found = []
        for item in value:
            found.extend(bindings(item))
        return found
    return []


def runtime_binding(data: dict[str, object]) -> dict[str, object] | None:
    for key in ("runtime", "runtime_evidence"):
        value = data.get(key)
        if isinstance(value, dict) and isinstance(value.get("path"), str) and \
                isinstance(value.get("sha256"), str):
            return value
    return None


def replayed_manifests() -> set[str]:
    result: set[str] = set()
    for line in REPLAY_LOG.read_text().splitlines():
        if not line.startswith("BEGIN ") or " manifests=" not in line:
            continue
        names = line.split(" manifests=", 1)[1]
        result.update(name for name in names.split(",") if name)
    return result


def main() -> None:
    if not REPLAY_LOG.is_file() or "AFFECTED_PROFILE_REPLAY_PASS" not in REPLAY_LOG.read_text():
        raise SystemExit("refresh refused: complete affected-profile replay log is missing")
    replayed = replayed_manifests()
    updated: list[str] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        data = json.loads(manifest.read_text())
        changed = False
        for section in ("images", "implementation"):
            for binding in bindings(data.get(section, {})):
                relative = str(binding["path"])
                path = ROOT / relative
                if not path.is_file():
                    raise SystemExit(f"refresh refused: missing {relative} in {manifest.name}")
                actual = sha256(path)
                if actual == binding["sha256"]:
                    continue
                if section == "implementation" and relative not in ALLOWLIST:
                    raise SystemExit(
                        f"refresh refused: non-allowlisted implementation mismatch "
                        f"{relative} in {manifest.name}"
                    )
                if section == "images" and manifest.name not in replayed:
                    raise SystemExit(
                        f"refresh refused: image mismatch without fresh replay in {manifest.name}"
                    )
                binding["sha256"] = actual
                changed = True
        runtime = runtime_binding(data)
        if runtime is not None and manifest.name in replayed:
            runtime_path = ROOT / str(runtime["path"])
            if not runtime_path.is_file():
                raise SystemExit(f"refresh refused: missing replay runtime {runtime_path}")
            actual = sha256(runtime_path)
            if actual != runtime["sha256"]:
                runtime["sha256"] = actual
                changed = True
        if changed:
            manifest.write_text(json.dumps(data, indent=2) + "\n")
            updated.append(manifest.name)
    print(f"EVIDENCE_BINDING_REFRESH_PASS count={len(updated)}")
    for manifest in updated:
        print(f"REFRESHED {manifest}")


if __name__ == "__main__":
    main()
