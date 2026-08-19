# Phase 78: fresh target mapping provenance M0 gate

## Status

**Status: VERIFIED.** The isolated `SeLinFreshTargetMappingProvenanceProbe=ON` QEMU TCG profile reached the exact suspended mapping-provenance marker and the root idle loop. All 51 profiles rebuilt, all evidence SHA-256 bindings were refreshed, and 84 standalone verifiers passed (excluding only the two documented host-dependent checks). This gate follows the rolled-back stack-consumption experiment. It does not retry `ret`, user execution, a stack read, fault repair, or a process startup. It establishes only that the existing mapping helper reported the expected fresh-VSpace hierarchy allocation ledger for the exact entry and stack virtual addresses.

> Phase 78 M0 is intentionally weaker than a translation proof. A successful mapping syscall and an allocation ledger do **not** prove that a future target instruction can access the page. They only make the mapping request, VSpace root, frame capability, target virtual address and missing-table allocation sequence evidence-visible before any execution retry.

## Preconditions

The profile must reproduce Phases 69–75 through fresh target construction, configuration and suspended full-context provenance. It must not execute the Phase 76 first-fetch path or Phase 77 reply/restart path. The fixed fresh VSpace already maps its IPC frame at `0x70000000` before Phase 78 M0 begins.

| Virtual address | x86_64 hierarchy indices | Expected state before Phase 78 M0 |
|---|---|---|
| IPC `0x70000000` | PML4 0, PDPT 1, PD 384, PT 0 | Phase 74 IPC mapping created the required hierarchy. |
| Stack `0x70002000` | PML4 0, PDPT 1, PD 384, PT 2 | Same final page table as IPC; stack leaf-map should require **zero** new paging objects. |
| Entry `0x60000000` | PML4 0, PDPT 1, PD 256, PT 0 | Shares PML4, PDPT and the existing PD; only a different final page-table slot is needed, so entry leaf-map should require **one** new paging object. |

## Required bounded transaction

| Step | Required action | Required result |
|---:|---|---|
| 1 | Check fixed Phase 74 IPC mapping address, Phase 75 RIP/RSP values, and fixed Phase 78 entry/stack addresses. | Any mismatch is rejected before mapping. |
| 2 | Map the fresh stack frame at `0x70002000` with `seL4_AllRights` and x86 `ExecuteDisable`, using `sel4utils_map_page_with_attributes`. | Return `seL4_NoError`; exact stack paging-object count is `0`. |
| 3 | Map the fresh entry frame RX at `0x60000000` with `sel4utils_map_page_with_attributes`. | Return `seL4_NoError`; exact entry paging-object count is `1`. |
| 4 | Emit one provenance marker and return while the fresh TCB stays suspended. | There is no `seL4_TCB_Resume`, `seL4_Recv`, `seL4_Reply`, user fault, stack alias, instruction payload, ELF, process, or Linux ABI operation. |

## Required evidence

The isolated default-OFF profile must have an independent protocol header, CMake option, QEMU TCG transcript, SHA-bound evidence manifest and standalone verifier. The verifier must scope only the Phase 78 M0 guard, assert stack-before-entry mapping order, `0` and `2` exact count checks, stack `ExecuteDisable`, entry default executable attributes, and structural absence of resume/receive/reply/payload/ELF operations. Historical Phase 74–77 verifiers must retain scopes bounded to their own guards.

## Explicit non-claims

Phase 78 M0 does not prove a present target PTE independent of the kernel’s map result, user read/write access, a return instruction, a stack word, executable fetch, fault delivery, process execution, ELF, dynamic linking, a general Linux process ABI, `dpkg`, `apt`, or Debian-package compatibility.
