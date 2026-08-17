# Phase 28: objectd fixed inventory M1 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `76ac0331e46d730f976e0903c408ba5d5ac934aaf4e50fe71635ba99b7bd9fe7`.** Objectd M1 proves only that an isolated objectd domain can own and transition a one-slot **reservation state** for future task objects. It does not create a TCB, CNode, endpoint, notification, fault endpoint, VSpace, mapping or any capability lease. The purpose is to validate the closed protocol and non-reuse policy before any allocation authority is considered.

> **No object exists merely because objectd reports a reserved inventory name.** M1 is not object creation, dynamic task creation, capability transfer or `clone` semantics.

## Exact protocol

The lifecycle probe receives one objectd endpoint and issues exactly two requests in sequence. The first request reserves the only slot. The second request repeats the reservation and must be rejected. Objectd replies synchronously in both cases; no indirect notification, callback or cap transfer exists.

| Request | Exact message words | Required reply | State transition |
|---|---|---|---|
| First reservation | `(SELINOS_OBJECTD_M1_RESERVE, slot=1)` | `SELINOS_OBJECTD_M1_RESERVED` | `FREE → RESERVED` |
| Duplicate reservation | `(SELINOS_OBJECTD_M1_RESERVE, slot=1)` | `SELINOS_OBJECTD_M1_EBUSY` | `RESERVED → RESERVED` |
| Any other shape | Any different label, length, opcode or slot | `SELINOS_OBJECTD_M1_EINVAL` or fail-closed no reply, chosen once in implementation. | No transition |

The probe succeeds only after observing `RESERVED` then `EBUSY` from the same server. The server emits the M1 success marker only after the exact sequence. It remains `RESERVED` until a separately designed abandon/release proof; M1 makes no reuse claim.

## Capability topology

Root creates only one fresh client-to-objectd endpoint and gives a copy to objectd and the objectd M1 probe. No extra server, taskd, child, memd or device cap participates. Objectd owns no VKA, untyped memory, TCB control, VSpace control, device frame, IRQ, DMA, PCI, IO port or IOSpace capability. This proof intentionally keeps root object allocation authority unchanged.

| Domain | Receives | Must not receive |
|---|---|---|
| objectd | One client endpoint. | VKA, untyped memory, taskd TCB cap, child CSpace/VSpace, endpoint creation authority, IRQ/DMA/device/PCI/I/O capability. |
| objectd M1 probe | One client endpoint. | Reservation authority other than requests, any task cap, any allocator/mapping/device capability. |
| Root | Construction-time endpoint and ordinary bootstrap references. | Runtime participation in the reservation state machine. |

## Required implementation and evidence

The generic objectd placeholder must be replaced by an M1-specific server source. A dedicated objectd M1 probe image must be added; neither generic `servers/service.c` nor the Linux syscall probe may be reused. Root must configure both isolated processes, copy the one endpoint to each at fixed verified CSpace slots, then spawn them. The QEMU proof must show the ordered acceptance, reserved reply, duplicate rejection and probe success markers.

The independent verifier must bind the image, root wiring, objectd server, probe, protocol header and QEMU log by SHA-256. It must assert that objectd contains no `vka_`, `vspace_`, `sel4utils_configure_process`, `seL4_Untyped_Retype`, `seL4_TCB_`, device/IRQ/DMA/PCI/I/O operation or Linux clone branch; and that root does not perform reservation transitions after spawning objectd.

## Promotion criteria

M1 is verified: `verify_objectd_fixed_inventory_m1.py` passed against the SHA-bound QEMU log `selinos_objectd_fixed_inventory_m1.boot.log`, and the complete default-plus-opt-in suite passed with 34 independent verifiers. The compatibility matrix states only: “objectd has a one-slot reservation-state proof with duplicate refusal.” It expressly retains the absence of object creation, cap lease, dynamic task, task teardown, clone, fork, pthread, VSpace, executable loading, glibc, `dpkg` and `apt` claims.
