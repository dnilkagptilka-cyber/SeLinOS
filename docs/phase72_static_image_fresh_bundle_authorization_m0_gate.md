# Phase 72: static-image fresh-bundle authorization M0 gate

## Status

**Status: VERIFIED.** Phase 72 follows verified Phase 71 terminal disposal. The default-OFF `SeLinStaticImageFreshBundleAuthorizationProbe=ON` profile completed the Phase 69 mapping-revocation, Phase 70 target-CNode deletion and Phase 71 root-held disposal sequence, then recorded one status-only authorization for logical fresh generation 2. Its duplicate generation-2 request and retired generation-1 request were rejected stale. It did not allocate, reallocate, construct, map, configure, resume, reply to, mint, copy or transfer any capability or object.

The isolated QEMU TCG transcript is SHA-bound in `tests/artifacts/selinos_static_image_fresh_bundle_authorization_m0.verification.json`. The independent verifier passed, the 44-profile rebuild passed, and the full standalone regression passed **78/78** verifiers.

> A fresh logical generation is not proof of resource reuse. The Phase 72 ledger authorizes only one future construction request identifier. It does not show an untyped region, CNode slot, ASID, target virtual address, object, frame or capability has actually become reusable.

## Required status transaction

| Step | Condition | Required status result | Stop/reject rule |
|---:|---|---|---|
| 1 | Terminal fault IP/vector match; Phase 71 terminal-disposal prerequisite is reached. | Eligibility is considered. | Any mismatch rejects. |
| 2 | Request has generation `2`, while retired generation is `1`, and authorization has not been consumed. | Return `APPROVED` and consume once. | No allocation or capability operation. |
| 3 | Repeat the same generation-2 request after consumption. | Return `REJECTED_STALE`. | No allocation, object/slot/ASID reuse or successor construction. |
| 4 | Request retired generation `1`. | Return `REJECTED_STALE`. | No allocation, object/slot/ASID reuse or successor construction. |

The default-OFF profile must first execute the Phase 69–71 prerequisite blocks. The independent verifier must require this block order, fresh-versus-retired generation checks, one approval and two rejections, and must reject every allocation, VKA free, CNode mutation, mapping, `TCB_Configure`, ASID action, reply, resume and successor construction in the Phase 72 block.

## Explicit non-claims

Phase 72 does not prove actual fresh allocation, untyped reuse, CNode-slot reuse, ASID reuse, target virtual-address reuse, resource reclamation correctness beyond Phase 71's isolated witness, process exit, wait/reap, process creation, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase71_static_image_object_reclamation_m0_gate.md` — verified one terminal root-held disposal sequence without reuse.
[2]: `docs/phase70_static_image_target_capability_deletion_m0_gate.md` — verified target-CNode copy deletion before disposal.
