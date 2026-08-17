# Phase 27: taskd dynamic fixed-child M2 proof gate

## Status

**Status: design gate only.** M2 may be attempted only after objectd and memd each have an independently auditable finite lease interface conforming to Phase 26. Current objectd and memd are placeholders; therefore this gate authorizes no implementation or compatibility claim yet.

## Single permitted scenario

One lifecycle probe submits exactly `CREATE_FIXED_CHILD(slot=1)`. Taskd obtains an object bundle and a mapping bundle for the same pinned generation, configures one fresh inert child with the fixed image and fixed stack plan, starts it, validates one exact completion and signals probe success. The child is then `TEARDOWN_PENDING`. No second create, reuse, normal exit, cleanup completion or resource release claim is permitted in M2.

| Step | Required owner | Required visible evidence | Fail-closed condition |
|---|---|---|---|
| Request | Probe → taskd | Exact `(CREATE_FIXED_CHILD, 1)` acceptance marker. | Any other message label, length, opcode or slot. |
| Object lease | taskd → objectd | `OBJECTS_READY(slot=1,generation=1)` marker only after a fixed lease is received. | Unknown/missing/duplicate/stale lease or any unexpected cap. |
| Mapping lease | taskd → memd | `VSPACE_READY(slot=1,generation=1)` marker only after matching fixed lease is received. | Missing/mismatched generation, variable layout or client-selected memory parameter. |
| Configuration | taskd | One marker after exact fixed child control, CSpace, VSpace, fault endpoint, entry and stack configuration. | A second configuration attempt, inherited TLS/FD/VFS state or arbitrary register/stack value. |
| Run/completion | taskd and child | `RUNNING` marker then exact `(slot=1,generation=1)` completion. | Child fault, malformed completion or activity before configuration. |
| End state | taskd | `TEARDOWN_PENDING` marker and probe success. | Slot reuse, cap release, object deletion or a general exit claim. |

## Mandatory authority constraints

Objectd may receive only the root-issued finite inventory for slot 1. Memd may receive only the root-issued fixed mapping inventory for slot 1. Taskd receives opaque lease caps only after each exact response and may not receive untyped, VKA, root CNode, global VSpace allocator, hardware frame, IRQ, PCI, IO port, IOSpace, DMA or device authority. The probe receives no lease or child cap.

The verifier must inspect the source tree for a closed lifecycle state machine, exact pinning of slot and generation, no Linux `clone` dispatcher branch, no client-supplied entry/stack/TLS/flags, and no allocator/VSpace/retype/device operations in taskd. It must bind every relevant source and a QEMU TCG log by SHA-256. The required log markers must be ordered; a substring-only collection of unrelated markers is insufficient.

## Promotion rule

M2 is verified only after a separate objectd lease proof, a separate memd lease proof, the combined QEMU proof, independent verifier, rebuilt opt-in profiles, evidence rebind and full regression suite. Even then the compatibility matrix may claim only one fresh fixed inert child from finite leases, ending in `TEARDOWN_PENDING`.

> M2 must not be described as `clone`, fork, pthread creation, process exit, child cleanup, wait, signal delivery, futex lifecycle, executable loader support, dynamic linker, `dpkg` or `apt` support.
