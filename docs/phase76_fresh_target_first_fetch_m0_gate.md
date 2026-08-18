# Phase 76: fresh target first-fetch M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinFreshTargetFirstFetchProbe=ON` isolated profile materialized `NOP; UD2` through a root-private alias into the fresh entry frame, mapped the entry executable at `0x60000000` and the stack NX at `0x70002000`, resumed the configured fresh TCB exactly once, and received the badged `UserException` at `0x60000001` with invalid-opcode vector `6`. The root withheld a fault reply and contains no second fresh-TCB resume in the Phase 76 block. The claim is bound to its QEMU TCG boot transcript and implementation hashes in `tests/artifacts/selinos_fresh_target_first_fetch_m0.verification.json`; a 48-profile rebuild and 82 standalone verifier regression passed.

> One terminal instruction fetch ending in a deliberate invalid-opcode fault is not a runnable process. It proves neither scheduling semantics, fault repair, a subsequent instruction, ELF loading, Linux ABI, `dpkg`, or `apt`.

| Step | Required action | Required boundary |
|---:|---|---|
| 1 | Materialize `NOP; UD2` in the fresh entry frame through a root-private alias, then unmap that alias. | No ELF parser, loader or copied executable image. |
| 2 | Map entry executable at `0x60000000` and stack NX at `0x70002000`. | No additional target mapping and no writable-executable mapping. |
| 3 | Resume the fresh TCB exactly once. | No reply, restart or second resume. |
| 4 | Receive and validate one badged user exception at `0x60000001`, vector `6`. | Terminal-only; no fault repair or successor. |

Phase 76 does not prove a runnable process, persistent scheduling, fault recovery, ELF, Linux ABI, `dpkg`, or `apt`.
