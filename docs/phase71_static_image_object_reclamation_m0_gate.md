# Phase 71: static-image object reclamation M0 gate

## Status

**Status: VERIFIED.** Phase 71 follows verified Phase 70 target-CNode capability deletion. The default-OFF `SeLinStaticImageObjectReclamationProbe=ON` profile reached the terminal invalid-opcode UserException, completed the Phase 69 three-leaf unmaps and Phase 70 six target-CNode deletions, then disposed of the one terminal target bundle only through root-held descriptors. It did not construct, resume, reply to, reconfigure, map into, or otherwise reuse a successor task.

The isolated QEMU TCG transcript is SHA-bound in `tests/artifacts/selinos_static_image_object_reclamation_m0.verification.json`. The independent verifier passed, the 43-profile rebuild passed, and the full standalone regression passed **77/77** verifiers.

> Object reclamation is not lifecycle reuse. The witness shows only one bounded disposal sequence completed in one isolated QEMU TCG profile. It does not establish that the same slots, untyped regions, ASID, virtual addresses or capabilities are safely reusable.

## Required transaction

| Step | Root-held object | Required operation | Dependency protected | Stop/reuse rule |
|---:|---|---|---|---|
| 1 | Target TCB | `vka_free_object` | Releases terminal TCB-held CSpace, VSpace, fault-endpoint and IPC-buffer references before their objects are disposed. | No reply, resume, reconfiguration or successor. |
| 2 | Entry, stack, IPC frames | `vka_free_object`, one frame at a time | Phase 69 already removed the target mappings; Phase 70 deleted target copies. | No frame mapping, allocation or replacement frame. |
| 3 | Intermediate paging objects | `vka_free_object` in reverse recorded allocation order | All leaf mappings are absent and the TCB is gone before paging hierarchy disposition. | No ASID recycle, PML4 replacement or mapping. |
| 4 | Target CNode | `vka_free_object` | All six target slots were deleted by Phase 70; TCB was disposed first. | No cap allocation, cap copy/mint/delete outside VKA disposal. |
| 5 | Target PML4 | `vka_free_object` | Leaf mappings and recorded intermediate paging objects were disposed. | No ASID recycle or VSpace reuse. |
| 6 | Fault endpoint, notification | `vka_free_object` | TCB and target CNode disposition precede endpoint/notification disposal. | No IPC, notification wait/signal or successor construction. |

The profile must be default-OFF and must reach the terminal vector/IP, complete the Phase 69 unmaps and Phase 70 target-CNode deletion block before entering this transaction. The verifier must require the exact phase ordering and root-object free order, require the no-reuse status marker, and reject post-terminal reply, resume, allocation, `TCB_Configure`, page mapping, ASID action, CNode copy/mint or successor construction.

## Explicit non-claims

Phase 71 does not prove deterministic untyped reuse, capability-slot reuse, ASID reuse or safe reuse of the target virtual addresses. It does not prove general process exit, wait/reap, process creation, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase70_static_image_target_capability_deletion_m0_gate.md` — verified deletion of six target-CNode capability copies while all root descriptors and objects remained retained.
[2]: `docs/phase69_static_image_mapping_revocation_m0_gate.md` — verified entry-stack-IPC target mapping revocation.
