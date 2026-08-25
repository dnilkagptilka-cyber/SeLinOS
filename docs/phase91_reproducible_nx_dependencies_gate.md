# Phase 91 — Reproducible x86 Execute-Disable Dependency Gate

**Status:** IMPLEMENTED — bootstrap and source-lock integration are validated by an independent static verifier and clean local builds. This gate is a dependency-integrity correction, not a new Linux-compatibility feature.

## Purpose

The x86_64 W^X probe profiles use the local `seL4_X86_ExecuteDisable` VM attribute and `sel4utils_map_page_with_attributes()` helper. Before this gate, SeLinOS kept the implementations of those interfaces in dedicated `seL4` and `seL4_libs` forks but `bootstrap_sources.sh` still selected the upstream commits that predate them. Consequently, a clean bootstrap could not be assumed to reconstruct the source tuple intended by the W^X profiles.

Phase 91 makes the intended tuple explicit and reproducible. `kernel/seL4` is pinned to `c7b5e55ec8cdddb506d69d96fdc69dc981088571` from the SeLinOS `selinos-x86-nx` fork branch, and `projects/seL4_libs` is pinned to `37b55704c1480ca6a8234cf962d01c099d20e7a1` from the matching fork branch. Each fork has the previous upstream lock revision as its direct parent. All other bootstrap inputs remain at the upstream `sel4test-manifest` baseline retrieved on 2026-08-13.

| Component | Pinned revision | Source role |
|---|---:|---|
| `kernel/seL4` | `c7b5e55ec8cdddb506d69d96fdc69dc981088571` | Defines and propagates the x86_64 execute-disable mapping attribute. |
| `projects/seL4_libs` | `37b55704c1480ca6a8234cf962d01c099d20e7a1` | Provides the attribute-aware page-mapping helper used by W^X probe profiles. |
| Other seL4 ecosystem inputs | Existing pinned upstream revisions | Preserve the previous reproducible project baseline. |

## Required evidence

The independent verifier checks that `bootstrap_sources.sh` and `sources.lock` select the exact two fork URLs and commit IDs, retain the complete expected source tuple, reject a regression to the former upstream selections, and bind both tracked files with SHA-256 evidence.

```bash
cd /path/to/SeLinOS
./bootstrap_sources.sh
./tools/verify_phase91_reproducible_nx_dependencies.py
cmake -S src -B build-phase91 -GNinja
cmake --build build-phase91 --parallel 2
cmake -S src -B build-phase91-nx -GNinja -DSeLinX86NxMappingProbe=ON
cmake --build build-phase91-nx --parallel 2
```

The clean default and `SeLinX86NxMappingProbe=ON` builds succeeded locally from the Phase 91 source tuple. This confirms compile-time integration of the two forked interfaces. A separately recorded QEMU runtime witness remains required for every behavior claim made by the existing W^X profile; this gate neither replaces nor reinterprets that evidence.

## Explicit non-claims

Phase 91 proves neither a new executable-mapping behavior nor a new ELF, `execve`, Linux syscall, dynamic-linker, filesystem, package-manager, driver, DMA-containment, `dpkg`, `apt`, Debian-package, or Debian 13.6 compatibility result. It does not promote any default-OFF profile to the production image. It only ensures that a clean bootstrap obtains the reviewed source implementations required by already-defined x86 execute-disable profiles.
