# Phase 66: static-image terminal lifecycle and ownership M0 gate

## Status

**Status: VERIFIED.** The QEMU TCG transcript, independent verifier, 38-profile rebuild and 72-standalone-verifier regression passed. Phase 66 begins from the verified Phase 65 terminal `ud2` witness. The target remains fault-blocked because root withholds the terminal UserException reply. This phase must make that terminal state and every root-held resource explicit before attempting any reuse, cleanup, or process-style lifecycle claim.

> A terminal user exception is not an exit protocol. No Phase 65 resource may be silently reclaimed, reused, or reissued on the basis of the terminal fault alone.

## Required ownership ledger

| Resource | Phase 65 terminal state | Phase 66 M0 permitted action |
|---|---|---|
| Target TCB | fault-blocked at `rip=0x60000001` | Observe terminal state only; no reply or second resume. |
| Entry frame | executable target mapping, no root writable alias | Record immutable terminal ownership; no remap/rewrite. |
| Stack and IPC frames | target mapped, stack NX | Record ownership only; no teardown claim. |
| Target CNode/PML4/ASID | configured target topology | Record exact linkage; no cap reuse or ASID recycle. |
| Fault endpoint | terminal sender blocked awaiting reply | Retain endpoint; do not issue UserException reply. |

A future implementation must emit a status-only ledger record after the terminal fault and prove that a duplicate observation is rejected. It must not free a frame, unmap target memory, delete a cap, suspend/resume the target, issue a reply, or create a successor task. Those actions need separate failure-safe cleanup gates.

## Explicit non-claims

Phase 66 does not prove exit, cleanup, resource reclamation, task reuse, process creation, ELF, general program execution, Linux process lifecycle, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase65_sealed_static_image_mapping_m0_gate.md` — verified one accepted sealed payload mapping and terminal invalid-opcode witness.

## Implementation handoff

The audited Phase 65 function currently returns immediately after validating the terminal UserException. Phase 66 must insert its status-only observation at that exact point. The implementation may inspect the already held root-side object descriptors for a fixed ledger, but must not invoke any deallocation, unmap, cap deletion, TCB control, reply or successor-construction API.

## Next implementation constraint

The first executable Phase 66 change must be default-OFF and may add only an exact observation record plus a duplicate-refusal check after the terminal fault validation. It must preserve the Phase 65 fault endpoint pending state and return without issuing a reply.

## Source-level invariant

The Phase 66 root bundle must contain zero calls to `seL4_Reply`, `seL4_TCB_Resume`, `vka_free_object`, `seL4_CNode_Delete`, `seL4_X86_Page_Unmap`, or successor construction after terminal fault validation. Its only new effect is formation of a fixed local ownership record for the already-held root-side descriptors.
