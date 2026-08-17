# Phase 51: taskd-owned dynamic TCB allocation M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdDynamicTcbOwnershipProbe=ON` established the narrow fact below after Phase 50: the root can allocate exactly one previously absent seL4 TCB object, move its sole live capability into a designated taskd CSpace slot, and obtain a taskd-issued ownership witness without invoking, configuring, resuming, mapping, or assigning an executable context to that target TCB.

> The taskd server and probe are pre-existing test scaffolding processes created only to run the bounded IPC proof. The Phase 51 **target TCB** is the sole object counted by this gate. It stays inert and inaccessible to the probe; executing scaffolding must never be mistaken for executing the target.

## Contract

The root performs the only allocation operation, `vka_alloc_tcb(vka, &target_tcb)`, after both scaffolding processes have their isolated CSpaces. It converts the root CSlot to a tracked `cspacepath_t` and calls `sel4utils_move_cap_to_process(&taskd, root_target_path, vka)`. A move—not a copy—is mandatory: the root VKA frees the source CSlot as part of the transfer, leaving the target TCB capability only in taskd's designated destination slot.

Taskd accepts only one exact two-word probe request and responds with a fixed three-word **owned** witness containing the fixed resource-plan slot and generation `1`. The root's checked `sel4utils_move_cap_to_process` return value and fixed taskd destination slot are the ownership-location evidence; taskd deliberately performs no CNode or TCB operation on the inert target. The probe checks the exact reply and signals success. Root then logs both the allocation/move witness and the explicit non-execution exclusions.

| Item | Required Phase 51 M0 property | Forbidden in Phase 51 M0 |
|---|---|---|
| Target object | Exactly one `seL4_TCBObject` allocated by the root VKA. | CNode, VSpace root, page table, frame, endpoint, notification or scheduler-context allocation for the target. |
| Capability ownership | Target TCB cap moves into one documented taskd CSpace slot; no copy is used. | Probe access to target cap; retained root target-cap slot; cross-domain delegation; any ambient root authority. |
| Ownership witness | Root checks that `sel4utils_move_cap_to_process` returned taskd's fixed destination slot; taskd then returns `OWNED`, fixed plan slot and generation `1` without touching the target object. | A claim of exclusive kernel-object ownership beyond the tracked CSpace location and root source-slot removal. |
| Target lifecycle | The target remains inert; no target TCB invocation occurs. | `seL4_TCB_Configure`, `seL4_TCB_SetSpace`, register writes, `seL4_TCB_Resume`, scheduling, endpoint binding, child code, process or thread creation from the target. |
| Failure policy | No success line if allocation, path construction or cap move fails; failure is fail-closed before probe spawn. | Partial success, retry loops that conceal state, cleanup that destroys a supposedly transferred target TCB, or Linux process semantics. |

## Bounded IPC protocol

The client request has two words: `OWNERSHIP_QUERY`, then the fixed resource-plan slot `1`. The server reply has three words: `OWNED`, slot `1`, generation `1`. Any wrong label, request length, operation, slot, badge-independent protocol shape, absent target cap, or second query is rejected with `REJECTED` and the same bounded reply shape. A single-use query prevents this gate from becoming a general task-control API.

The target TCB capability is not returned in an IPC reply. The probe learns only the status witness. This preserves the purpose of the gate: verify taskd's controlled CSpace possession, not make the test client a co-owner.

## Verified evidence

The clean QEMU TCG transcript records the allocation/move, root source-slot-release claim, taskd non-invoking ownership witness and probe success. `tests/artifacts/selinos_taskd_dynamic_tcb_m0.verification.json` binds that transcript, generated kernel/root image, protocol, server, probe, root wiring, CMake gate and this document by SHA-256. `tools/verify_taskd_dynamic_tcb_m0.py` independently checks the binding, the audited move-not-copy source contract and absence of target execution/configuration primitives. Promotion also completed a successful all-profile rebuild (**23 / 23**) and a fresh standalone verifier regression (**57 / 57**).

## Evidence required for promotion

The promotion record must bind the two generated images, Phase 51 protocol header, server, probe, root wiring, CMake gate, QEMU transcript and this design gate by SHA-256. The independent verifier must reject evidence if the source lacks `vka_alloc_tcb`, lacks `sel4utils_move_cap_to_process`, uses a cap-copy helper for the target, fails to name a move/source-slot-removal witness, or contains any target-execution primitive.

The QEMU transcript must contain all markers below, with exact text under verifier control.

```text
SeLinOS taskd dynamic TCB M0: target TCB allocated then moved into taskd ownership slot.
SeLinOS taskd dynamic TCB M0: target TCB remains unconfigured, unmapped and unresumed.
SeLinOS taskd dynamic TCB M0: ownership query passed; one target TCB owned by taskd; no target executed.
```

## Explicit exclusions

This gate does not establish dynamic CSpace/VSpace construction, root authority revocation, object destruction/reclamation, allocator rollback after a post-allocation fault, child execution, thread scheduling, Linux `clone`, `fork`, `vfork`, `pthread`, PID/TID allocation, TLS, signals, `wait`, `execve`, ELF loading, filesystems, storage, networking, `dpkg`, `apt`, Linux driver execution, or general Linux ABI compatibility. These require separate evidence gates.
