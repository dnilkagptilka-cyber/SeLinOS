# Phase 52: taskd-owned dynamic CNode and rollback M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdDynamicCspaceRollbackProbe=ON` follows Phase 51 and established only that the root VKA can allocate a small CNode object, take one intentional **pre-delegation policy-reject** branch that destroys that root-owned object through `vka_free_object`, then allocate one replacement CNode and move its sole live capability into taskd's fixed CSpace slot.

> The rollback event is deliberately injected by this test profile after a successful allocation and before any transfer. It proves the local root-side cleanup sequence, not a general allocator-failure recovery mechanism. The final CNode is an inert capability-table object and must never become a TCB CSpace root or receive a capability.

## Contract

The root creates the server/probe scaffolding, endpoint and completion notification as fixed test infrastructure. It then performs these resource-plan events in order.

| Generation | Root operation | Required result | Delegation state |
|---|---|---|---|
| 1 | `vka_alloc_cnode_object(vka, slot_bits, &rollback_cnode)` | One CNode object exists only in root VKA accounting. | The policy rejects it before any CSpace move; root calls `vka_free_object(vka, &rollback_cnode)`. |
| 2 | `vka_alloc_cnode_object(vka, slot_bits, &owned_cnode)` | One replacement CNode object exists only in root VKA accounting. | Root converts its source CSlot to `cspacepath_t` and calls `sel4utils_move_cap_to_process(&taskd, root_owned_path, vka)`. |
| 2 witness | Taskd accepts one exact query. | Replies `OWNED`, plan slot `1`, generation `2`; a duplicate receives `REJECTED`. | Final CNode capability remains taskd-local in the documented slot; probe receives no CNode capability. |

A move—not a copy—is mandatory for the final CNode. Root checks the exact destination CSpace slot returned by the move helper. The server and probe perform no CNode invocation on the final object; the CNode remains empty and inert.

## Security invariants

| Invariant | Required evidence | Explicitly excluded |
|---|---|---|
| Root-side rollback before delegation | Source contains `vka_free_object` on the generation-1 object before the generation-2 allocation/move; QEMU records the rollback witness. | Post-delegation recovery, reparenting or destruction of the final taskd-owned CNode. |
| Narrow authority transfer | Final cap moves to one taskd CSpace slot; no final-CNode cap is copied to probe. | CNode capability fan-out, rights derivation, cross-domain delegation or shared authority. |
| No task CSpace construction | Neither target TCB nor any process uses the final CNode as its active CSpace. | `seL4_TCB_SetSpace`, TCB configuration, cap insertion into the final CNode, child setup or execution. |
| Bounded protocol | One exact request yields `OWNED`/generation `2`; the duplicate is rejected. | General allocator API, process manager, `clone`, `fork`, `vfork` or `pthread` semantics. |

## Required runtime markers

```text
SeLinOS taskd dynamic CSpace M0: root rolled back rejected CNode generation 1 before delegation.
SeLinOS taskd dynamic CSpace M0: replacement CNode generation 2 allocated then moved into taskd ownership slot.
SeLinOS taskd dynamic CSpace M0: final CNode remains empty, inert and is not a TCB CSpace root.
SeLinOS taskd dynamic CSpace M0: rollback then ownership query passed; no task CSpace configured.
```

## Verified evidence

The clean QEMU TCG transcript records the injected generation-1 root-side rollback, generation-2 CNode move into taskd, non-invoking ownership witness and probe success. `tests/artifacts/selinos_taskd_dynamic_cspace_m0.verification.json` binds the transcript, generated kernel/root image, protocol, server, probe, root wiring, CMake gate and this document by SHA-256. `tools/verify_taskd_dynamic_cspace_m0.py` independently checks the rollback-before-move source ordering and rejects active task-CNode use. Promotion completed a successful all-profile rebuild (**24 / 24**) and fresh standalone regression (**58 / 58**).

## Promotion evidence

A promotion record must bind the generated kernel/root images, protocol header, server, probe, root wiring, CMake gate, QEMU transcript and this gate by SHA-256. The independent verifier must require the generation-1 `vka_free_object` before the generation-2 move and reject sources that configure a TCB, insert/copy/move capability into the final CNode, resume a target TCB, construct Linux process semantics, or give the probe the final CNode cap.

## Explicit exclusions

This gate does not establish dynamic task CSpace construction, creating an actual child CSpace hierarchy, CNode slot allocation inside the final object, CNode revocation, post-delegation rollback, CSpace destruction, CSpace reuse, target TCB linkage, VSpace/page-table/frame allocation, child execution, scheduling, ELF loading, Linux `clone`/`fork`/`pthread`, signals, wait/reap, filesystems, storage, networking, `dpkg`, `apt`, Linux driver execution or package compatibility.
