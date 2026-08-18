# Phase 69: static-image mapping revocation M0 gate

## Status

**Status: VERIFIED.** Phase 69 follows the verified Phase 68 ordering authorization. The default-OFF `SeLinStaticImageMappingRevocationProbe=ON` profile reached the terminal invalid-opcode UserException and then invoked `seL4_X86_Page_Unmap` successfully for exactly the target entry, stack and IPC frame caps in that order. The root retained every frame, target TCB, target CNode, target PML4, endpoint and paging object; it did not reply to the terminal UserException, resume the target, delete a capability, free an object, recycle an ASID or construct a successor.

The isolated QEMU TCG transcript is SHA-bound in `tests/artifacts/selinos_static_image_mapping_revocation_m0.verification.json`. The independent verifier passed, the 41-profile rebuild passed, and the full standalone regression passed **75/75** verifiers.

> Removing a mapping is not resource reclamation. This witness establishes only that the three specified `Page_Unmap` calls returned success in the one terminal target VSpace transaction. It does not independently read back page tables and does not prove safe object disposition or reuse.

## Required transaction

| Step | Mapping | Address | Required result | Rollback/stop rule |
|---:|---|---:|---|---|
| 1 | Entry frame | `0x60000000` | Exact target mapping unmap succeeds. | On error, stop; no later mapping/cap/frame action. |
| 2 | Stack frame | `0x70002000` | Exact target mapping unmap succeeds. | On error, stop; no later mapping/cap/frame action. |
| 3 | IPC frame | `0x70000000` | Exact target mapping unmap succeeds. | On error, stop; no later mapping/cap/frame action. |
| 4 | Evidence boundary | all three | The independent verifier binds the source order, three unmap calls, QEMU markers and isolated-image SHA-256 values. | No separate PTE readback or negative-remap assertion is made in M0; no PML4 deletion, cap deletion, frame free or reallocation. |

The transaction is reachable only in a default-OFF profile and only after the already validated terminal vector/IP plus non-null descriptor ledger. The verifier rejects a reply, resume, free, capability deletion, ASID action, allocation, reuse or successor construction in the Phase 69 post-terminal block.

## Explicit non-claims

Phase 69 does not prove a general VSpace teardown, page-table reclamation, capability deletion, frame reclamation, ASID reuse, task reuse, process exit, process creation, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase68_static_image_mapping_revocation_authorization_m0_gate.md` — verified entry-stack-IPC revocation ordering authorization without unmap.
[2]: `docs/phase67_static_image_teardown_authorization_m0_gate.md` — verified terminal teardown authorization without teardown.
