# Phase 54: taskd-owned inert task-resource bundle and rollback M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdInertBundleRollbackProbe=ON` integrates the separately proven Phase 51–53 primitives into exactly one resource bundle: an inert `seL4_TCBObject`, a small `seL4_CapTableObject`, and an x86_64 PML4 VSpace-root object. The gate must prove root-side generation-1 cleanup before any delegation, followed by generation-2 move—not-copy delivery of all three final caps to documented taskd slots.

> A resource bundle is **not a task**. At every point in this gate, the final TCB is unconfigured and unresumed; the final CNode remains empty and is not an active CSpace; the final PML4 is unassigned and unmapped; and no `seL4_TCB_Configure` or `seL4_TCB_SetSpace` connects the objects.

## Contract

| Generation | Resource sequence | Required result | Prohibited state transition |
|---|---|---|---|
| 1 | Root allocates one TCB, one 16-slot CNode and one x86_64 VSpace root; a deliberate policy reject follows only after all three allocations succeed. | Root calls `vka_free_object` for PML4, CNode and TCB before a capability move or process spawn. | Any delegation, configuration, mapping, invocation or status witness for generation 1. |
| 2 | Root allocates a replacement TCB, CNode and PML4. | Root turns each source CSlot into a `cspacepath_t` and calls `sel4utils_move_cap_to_process` for taskd slots 9, 10 and 11, respectively. | Copying final caps, probe access to any final cap, cap insertion into the final CNode or PML4 use. |
| 2 witness | Taskd accepts one exact status query, returns `OWNED`, plan slot `1`, generation `2`, then rejects a duplicate. | The probe receives only a status record and signals success. | An allocator, process-manager or task-control protocol. |

The final move slots are fixed: server endpoint `8`, TCB `9`, CNode `10`, and PML4 `11`. The root checks all exact destination slots before spawning the bounded server and probe scaffolding. The probe has only endpoint and notification caps at its slots `8` and `9`.

## Security invariants

| Invariant | Required evidence | Explicitly excluded |
|---|---|---|
| Atomic pre-delegation boundary | Generation-1 frees appear after all three allocations and before any final resource move. | Rollback after delegation, transactional recovery from arbitrary kernel errors, object reclamation after use or resource reuse. |
| Non-shared authority | The final three source caps move into taskd CSpace; the probe receives no final-object capability. | Root retained copies, capability fan-out, cross-domain sharing, rights derivation or delegation by taskd. |
| Mutually unlinked resources | No target TCB, CNode or PML4 invocation occurs. | TCB configuration/space association, ASID assignment, page-table/frame allocation, mapping, registers, scheduling or execution. |
| Bounded observation | One owned witness then one duplicate rejection uses only status words. | Linux `clone`, `fork`, `vfork`, `pthread`, PID/TID, TLS, signals, wait/reap, `execve` or general Linux ABI. |

## Required runtime markers

```text
SeLinOS taskd inert bundle M0: root rolled back rejected TCB/CNode/PML4 generation 1 before delegation.
SeLinOS taskd inert bundle M0: replacement TCB/CNode/PML4 generation 2 allocated then moved into taskd ownership slots.
SeLinOS taskd inert bundle M0: final resources remain mutually unlinked, inert and unexecuted.
SeLinOS taskd inert bundle M0: rollback then bundle ownership query passed; no task constructed.
```

## Verified evidence

The clean QEMU TCG transcript records generation-1 root-side reverse-order rollback after all three allocations, generation-2 move-only delivery of TCB/CNode/PML4 caps into taskd, a non-invoking ownership witness and probe success. `tests/artifacts/selinos_taskd_inert_bundle_m0.verification.json` binds the transcript, generated kernel/root images, protocol, server, probe, root wiring, CMake gate and this document by SHA-256. `tools/verify_taskd_inert_bundle_m0.py` independently enforces rollback ordering, three fixed move destinations and the no-linkage/no-invocation boundary. Promotion completed a successful all-profile rebuild (**26 / 26**) and fresh standalone regression (**60 / 60**).

## Promotion evidence

The promotion record must bind protocol, server, probe, root wiring, CMake gate, generated kernel/root images, QEMU transcript and this document by SHA-256. The independent verifier must enforce each generation-1 allocation/free marker, its ordering before every generation-2 cap move, all fixed destination slots, move-not-copy transfer, and the absence of target-object configuration, CNode operations, PML4/ASID/mapping primitives and execution operations.

## Explicit exclusions

This gate does not establish a dynamic child task. It excludes task-resource linkage, TCB CSpace/VSpace configuration, ASID assignment, CNode population, page-table/frame allocation, mappings, W^X, ELF loading, process startup, process teardown/reuse, schedulable child execution, Linux `clone`/`fork`/`pthread`, dynamic linking, filesystem persistence, storage, networking, package trust, `dpkg`, `apt`, Linux driver execution and Debian/Ubuntu package compatibility.
