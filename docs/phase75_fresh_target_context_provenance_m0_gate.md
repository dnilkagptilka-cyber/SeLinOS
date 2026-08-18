# Phase 75: fresh target context provenance M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinFreshTargetContextProvenanceProbe=ON` profile completed the Phase 69–74 prerequisite chain, completed one fresh suspended target context write/readback, passed isolated QEMU TCG evidence, independent verification, a **47-profile** rebuild and **81/81** standalone verifiers. It establishes neither resume, execution, ASID reuse nor Linux compatibility.

**Design basis:** Phase 75 follows verified Phase 74 suspended bundle configuration. It may write and read back one complete x86_64 user context for the fresh target TCB, with fixed non-executable entry/stack provenance values retained only as register data. The target remains suspended.

> Register write/readback is not instruction fetch. This phase establishes neither executable mapping, resume, fault delivery, target execution, process lifecycle, ELF nor Linux ABI behavior.

| Step | Required action | Required boundary |
|---:|---|---|
| 1 | Prepare one complete zero-initialized `seL4_UserContext` with fixed canonical RIP and ABI-aligned RSP values. | No frame write, mapping or target invocation. |
| 2 | Write all architecture context words to the fresh configured TCB. | No `TCB_Resume`, reply or scheduler action. |
| 3 | Read all words back and compare RIP/RSP plus zeroed control fields. | No execution or fault interaction. |
| 4 | Emit one suspended-context marker. | No cleanup, reuse or successor construction. |

Phase 75 does not prove execution, executable entry mapping, entry/stack contents, ASID reuse, ELF, Linux ABI, `dpkg` or `apt`.
