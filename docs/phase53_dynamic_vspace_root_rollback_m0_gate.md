# Phase 53: taskd-owned dynamic VSpace-root and rollback M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdDynamicVspaceRollbackProbe=ON` follows the Phase 51 TCB and Phase 52 CNode ownership primitives and established only that root can allocate an x86_64 VSpace root through `vka_alloc_vspace_root`, intentionally reject and free generation 1 while still root-owned, then allocate generation 2 and move its sole live PML4 capability into taskd's fixed CSpace slot.

> On x86_64, the top-level seL4 VSpace object is a PML4, while PDPT, page-directory and page-table objects are separate intermediate objects for mappings.[1] This gate allocates neither intermediate paging object nor frame, makes no PML4 invocation and never assigns the final PML4 to a TCB.

## Contract

| Generation | Root operation | Required result | Forbidden transition |
|---|---|---|---|
| 1 | `vka_alloc_vspace_root(vka, &rollback_vspace_root)` followed by deliberate policy reject and `vka_free_object`. | The root-side pre-delegation rollback path is exercised before any CSpace transfer. | Transfer, ASID assignment, PML4 map/unmap invocation, TCB linkage or process setup. |
| 2 | `vka_alloc_vspace_root(vka, &owned_vspace_root)` and `sel4utils_move_cap_to_process(&taskd, root_owned_path, vka)`. | Exactly one replacement PML4 cap moves—not copies—to documented taskd slot. | Probe access, root copy retention, CNode fan-out or use as an active VSpace. |
| 2 witness | One exact status query returns `OWNED`, plan slot `1`, generation `2`; duplicate returns `REJECTED`. | Taskd acknowledges the root-checked ownership location without invoking the PML4. | General allocator API or a process-management protocol. |

The final PML4 capability remains inert. Taskd, probe and root must not invoke it, assign it an ASID, associate it with a TCB, create PDPT/page-directory/page-table/frame objects, map or unmap any virtual address, configure registers, resume a target TCB, or load code.

## Required runtime markers

```text
SeLinOS taskd dynamic VSpace M0: root rolled back rejected VSpace root generation 1 before delegation.
SeLinOS taskd dynamic VSpace M0: replacement VSpace root generation 2 allocated then moved into taskd ownership slot.
SeLinOS taskd dynamic VSpace M0: final x86_64 PML4 remains inert, unassigned and unmapped.
SeLinOS taskd dynamic VSpace M0: rollback then ownership query passed; no active VSpace configured.
```

## Verified evidence

The clean QEMU TCG transcript records generation-1 root-side PML4 rollback, generation-2 PML4 move into taskd, a non-invoking taskd witness and probe success. `tests/artifacts/selinos_taskd_dynamic_vspace_m0.verification.json` binds transcript, generated kernel/root images, protocol, server, probe, root wiring, CMake gate and this document by SHA-256. `tools/verify_taskd_dynamic_vspace_m0.py` independently checks rollback-before-move ordering and excludes ASID, mapping and TCB-linkage primitives. Promotion completed a successful all-profile rebuild (**25 / 25**) and fresh standalone regression (**59 / 59**).

## Promotion evidence

The evidence record must bind protocol, server, probe, root wiring, CMake gate, both generated images, QEMU transcript and this gate with SHA-256. The independent verifier must require generation-1 `vka_free_object` before the generation-2 final-cap move; reject a final-cap copy; and reject VSpace/TCB/mapping invocations in the isolated Phase 53 bundle and service sources.

## Explicit exclusions

This gate does not establish a VSpace layout, ASID assignment, PDPT/page-directory/page-table/frame lifecycle, mapping or unmapping, W^X policy enforcement, `PT_LOAD`, ELF relocation, dynamic linking, target TCB linkage, process execution, scheduling, `clone`, `fork`, `vfork`, `pthread`, signals, storage, networking, Linux driver execution, `dpkg`, `apt` or Debian/Ubuntu package compatibility.

## References

[1]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
