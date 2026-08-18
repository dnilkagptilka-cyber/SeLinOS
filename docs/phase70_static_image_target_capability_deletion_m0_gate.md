# Phase 70: static-image target capability deletion M0 gate

## Status

**Status: VERIFIED.** Phase 70 follows verified Phase 69 mapping revocation. The default-OFF `SeLinStaticImageTargetCapabilityDeletionProbe=ON` profile reached the terminal invalid-opcode UserException, completed Phase 69's entry-stack-IPC `Page_Unmap` calls, then successfully deleted only the six target-CNode copies: notification slot 1, IPC-frame slot 2, fault-endpoint slot 3, entry-frame slot 4, stack-frame slot 5, and self-CNode slot 0 last. The root retained its direct capabilities to the endpoint, TCB, target CNode, target PML4, notification, IPC frame, entry frame, stack frame, and all paging objects.

The isolated QEMU TCG transcript is SHA-bound in `tests/artifacts/selinos_static_image_target_capability_deletion_m0.verification.json`. The independent verifier passed, the 42-profile rebuild passed, and the full standalone regression passed **76/76** verifiers.

> Deleting a cap copy from the target CNode is not object reclamation and does not establish a clean TCB/VSpace teardown. This phase does not invoke `vka_free_object`, delete a root-held capability, release paging objects, recycle an ASID, resume or reply to the target, or construct/reuse a successor task.

## Required transaction

| Step | Target CNode slot | Capability copy | Required result | Stop rule |
|---:|---:|---|---|---|
| 1 | 1 | Notification | `seL4_CNode_Delete` succeeds. | On error, stop; no later cap or object action. |
| 2 | 2 | IPC frame | `seL4_CNode_Delete` succeeds. | On error, stop; no later cap or object action. |
| 3 | 3 | Badged fault endpoint | `seL4_CNode_Delete` succeeds. | On error, stop; no later cap or object action. |
| 4 | 4 | Entry frame | `seL4_CNode_Delete` succeeds. | On error, stop; no later cap or object action. |
| 5 | 5 | Stack frame | `seL4_CNode_Delete` succeeds. | On error, stop; no later cap or object action. |
| 6 | 0 | Self-CNode | `seL4_CNode_Delete` succeeds last. | Stop; no object free, cap reuse or successor action. |

The default-OFF profile must reach Phase 69's terminal vector/IP and complete its entry-stack-IPC `Page_Unmap` calls before it attempts deletion. A post-transaction status ledger must retain non-null root descriptors for the target TCB, target CNode, target PML4 and three frames. The independent verifier must bind the target-only slot list and order, prove no root-CNode deletion, and reject reply, resume, object free, ASID action, allocation, mapping, successor construction or reuse in the Phase 70 block.

## Explicit non-claims

Phase 70 does not prove global capability revocation, root capability deletion, object destruction, frame or page-table reclamation, ASID reuse, task reuse, process exit, process creation, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase69_static_image_mapping_revocation_m0_gate.md` — verified terminal entry-stack-IPC target mapping revocation without capability deletion or reclamation.
[2]: `docs/phase68_static_image_mapping_revocation_authorization_m0_gate.md` — verified status-only entry-stack-IPC revocation-order authorization.
