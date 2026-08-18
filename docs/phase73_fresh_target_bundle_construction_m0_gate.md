# Phase 73: fresh target-bundle construction M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinFreshTargetBundleConstructionProbe=ON` profile completed the Phase 69–72 prerequisite chain, allocated one root-owned inert generation-2 TCB/CNode/PML4/notification/IPC/entry/stack bundle, passed independent verification, the 45-profile rebuild and the **79/79** standalone verifier regression. It makes no physical-reuse, configuration or execution claim.

**Design basis:** Phase 73 follows verified Phase 72 status-only authorization. It may allocate exactly one fresh inert target bundle consisting of a TCB, small CNode, x86_64 PML4, notification, IPC frame, entry frame and stack frame. The root retains all resulting descriptors. The target remains unconfigured, without ASID assignment, mappings, target-CNode copies, registers, fault endpoint, scheduling parameters, resume or execution.

> Successful fresh allocation does not demonstrate physical reuse of any Phase 71-disposed object. The evidence identifies a fresh construction transaction only; it does not establish untyped-region, slot, ASID, virtual-address or capability reuse.

## Required transaction

| Step | Fresh object | Required result | Stop rule |
|---:|---|---|---|
| 1 | TCB | Root VKA allocates one non-null inert target TCB descriptor. | On error, stop; no subsequent construction action. |
| 2 | CNode and PML4 | Root VKA allocates one small CNode and one PML4 descriptor. | No ASID assignment, CNode copy/mint or TCB configuration. |
| 3 | Notification and three frames | Root VKA allocates notification plus IPC, entry and stack frames. | No mapping, frame write, VSpace operation or capability transfer. |
| 4 | Ownership record | All seven descriptors are non-null and fresh generation `2` is recorded once. | No target invocation, reuse assertion or successor construction. |

The profile must reach Phase 72's successful fresh-generation authorization after executing Phases 69–71. The independent verifier must bind this prerequisite ordering, seven exact VKA allocation calls and one root-held ownership record, and reject VKA free, CNode mutation, ASID operation, mapping, TCB configuration, reply, resume, frame writes, capability transfer or execution in the Phase 73 block.

## Explicit non-claims

Phase 73 does not prove physical resource reuse, ASID assignment or reuse, target CSpace/VSpace linkage, target capability population, IPC buffer configuration, mappings, register context, fault handling, execution, process lifecycle, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase72_static_image_fresh_bundle_authorization_m0_gate.md` — verified status-only fresh-generation authorization and stale-request rejection without allocation.
[2]: `docs/phase71_static_image_object_reclamation_m0_gate.md` — verified terminal target-bundle disposition without reuse.
