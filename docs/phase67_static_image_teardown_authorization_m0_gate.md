# Phase 67: static-image terminal teardown authorization M0 gate

## Status

**Status: VERIFIED.** The QEMU TCG transcript, independent verifier, 39-profile rebuild and 73-standalone-verifier regression passed. Phase 67 follows the verified Phase 66 ownership observation. It defines the authorization preconditions for a future teardown but performs no teardown operation. The target remains blocked in its terminal UserException state; all root-held descriptors remain owned and unchanged.

> Authorization is not teardown. A successful Phase 67 authorization record does **not** release an object, unmap memory, delete a capability, reply to the fault, resume a TCB, recycle an ASID, or create a successor task.

## Required authorization record

| Required fact | Source | M0 decision |
|---|---|---|
| Terminal fault IP equals `0x60000001` | Phase 65/66 terminal witness | Required. |
| Terminal vector equals invalid opcode (`6`) | Phase 65/66 terminal witness | Required. |
| TCB, CNode, PML4, entry, stack and IPC descriptors are non-null | Phase 66 ownership record | Required. |
| Entry remains separate from initial root writable alias | Phase 65 W^X ledger | Required. |
| No reply and no second resume have occurred | Phase 65/66 source boundary | Required. |
| Duplicate authorization request | service-local state | Reject, with no state mutation. |

The permitted positive M0 outcome is a fixed status-only **authorization token** that remains local to root. An altered terminal IP, altered vector, missing descriptor, or duplicate request must be rejected. Neither result grants a capability, changes an object, or schedules a teardown.

## Future teardown order (not implemented)

Any later teardown proof must establish revocation-safe ordering independently: preserve target non-execution; remove target mappings; prove page-table object disposition; delete target-owned caps; release object backing only after no remaining mappings/caps; then separately establish any allocation/reuse authorization. Phase 67 does not implement or prove any element of that sequence.

## Explicit non-claims

Phase 67 does not prove teardown, cleanup, reclamation, object reuse, process exit, process creation, ELF execution, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase66_static_image_terminal_lifecycle_m0_gate.md` — verified terminal ownership observation and no-reply boundary.
[2]: `docs/phase65_sealed_static_image_mapping_m0_gate.md` — verified one-frame W^X materialization and terminal witness.
