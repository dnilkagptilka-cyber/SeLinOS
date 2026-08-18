# Phase 68: static-image mapping revocation authorization M0 gate

## Status

**Status: VERIFIED.** The QEMU TCG transcript, independent verifier, 40-profile rebuild and 74-standalone-verifier regression passed. Phase 68 follows verified terminal ownership (Phase 66) and terminal teardown authorization (Phase 67). It authorizes only the *ordering precondition* for a future revocation of the three target mappings. It does not issue any `Page_Unmap`, page-table deletion, capability deletion, object free, ASID recycle, target reply, target resume, or successor construction.

> Mapping revocation authorization is not a mapping revocation. The target retains all three mappings and remains blocked on the terminal UserException throughout this gate.

## Required mapping ledger

| Mapping | Fixed virtual address | Required authorization fact | Future order |
|---|---:|---|---:|
| Entry | `0x60000000` | Its frame descriptor is non-null, source policy is `R|X`, and the terminal IP is `0x60000001`. | First candidate. |
| Stack | `0x70002000` | Its frame descriptor is non-null and the established mapping policy is NX. | Second candidate. |
| IPC buffer | `0x70000000` | Its frame descriptor is non-null and target TCB remains non-executing. | Third candidate. |

The only M0 positive result is a root-local sequence value marking that all exact facts passed. A malformed order, altered terminal vector or IP, absent mapping descriptor, or a second authorization attempt must receive the fixed rejection result without state mutation.

## Explicit non-claims

Phase 68 does not prove a mapping revocation, flush, cleanup, page-table disposition, capability deletion, object free, resource reuse, process exit, process creation, ELF, Linux ABI, `dpkg`, or `apt`.

## References

[1]: `docs/phase67_static_image_teardown_authorization_m0_gate.md` — verified terminal teardown authorization without teardown.
[2]: `docs/phase66_static_image_terminal_lifecycle_m0_gate.md` — verified terminal ownership observation.
