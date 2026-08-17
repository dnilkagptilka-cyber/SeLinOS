# Phase 31: opaque lease-cap transfer M1 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `523431978ebca5ecc3f70db4f32d742690c88aadddf58e090a1968462a09a7f0`.** Phase 30 proves that taskd can coordinate status-only reservation replies. The next narrow increment is a one-shot **opaque lease-cap transfer** from a dedicated objectd lease service to a dedicated taskd lease service. The transferred cap must not name or control a child TCB, CSpace, VSpace, frame, mapping, endpoint used by any existing subsystem, device, IRQ, DMA, PCI, I/O or IOSpace.

> The M1 token is a root-preprovisioned notification capability whose only purpose is to make cross-CSpace capability transfer observable and auditable. Possession is not object allocation and does not authorize task creation.

## Exact bounded protocol

The dedicated taskd lease service accepts one exact client request `REQUEST_OPAQUE_LEASE(slot=1)`. It calls a dedicated objectd lease service. Objectd validates its one-slot state, sends one notification-token cap in its reply, and transitions from `FREE` to `TRANSFERRED`. Taskd receives the cap only into a single predeclared CSpace slot, validates the reply shape, and reports success to a dedicated probe notification. A repeated request is rejected or remains unserviced according to an explicitly chosen fail-closed policy; M1 makes no release/reuse claim.

| Element | Required M1 rule | Explicitly excluded |
|---|---|---|
| Token source | Root creates one fresh notification and copies it only to the dedicated objectd lease service before start. | Task object, TCB, CSpace, VSpace, page table, frame, mapping, device and generic allocator caps. |
| Transfer mechanism | Objectd reply contains exactly one extra cap; taskd configures an exact receive path for one named empty CSpace slot. | Client-provided destination, multi-cap reply, cap minting, rights amplification, forwarding to probe or arbitrary CSpace modification. |
| Taskd state | `EMPTY → TOKEN_RECEIVED`; one success notification after exact response. | Configuring, starting or controlling a child; interpreting the token as a resource bundle. |
| Objectd state | `FREE → TRANSFERRED`; no second token reply. | Revoke/delete/reuse, allocator operations, resource creation or a general object service. |
| Probe | Sends one fixed request then waits for one fixed notification. | Receiving, naming or using the transferred capability. |

## Required topology

This must be a new dedicated four-domain bundle: taskd-lease-M1, objectd-lease-M1, client probe and no additional child. Root creates one client endpoint, one taskd-to-objectd endpoint, one probe-success notification and the opaque token notification. Root copies only the correct direction-specific endpoints and notification caps into fixed slots. The objectd lease service receives the token only in an explicitly documented source slot; taskd receives it only in an explicitly documented destination slot. Existing Phase 25, 28, 29 and 30 bundles are not modified or reused.

## Mandatory verifier checks

The verifier must bind the image, root wiring, both service sources, probe, protocol header and QEMU log by SHA-256. It must require source evidence for `seL4_SetCap` on the objectd reply, a fixed receive path in taskd, exact message length/extra-cap count, and exact token destination slot. It must reject VKA, untyped, VSpace, retype, TCB, child configuration, device, IRQ, DMA, PCI, I/O, IOSpace and Linux clone operations in every M1 service. The QEMU log must show ordered client acceptance, objectd transfer, taskd receipt and probe success markers.

## Promotion rule

M1 is verified: `verify_opaque_lease_cap_transfer_m1.py` passed against the SHA-bound QEMU log `selinos_opaque_lease_cap_transfer_m1.boot.log`, and the complete default-plus-opt-in suite passed with 37 independent verifiers. The compatibility matrix claims only: “taskd received one opaque root-preprovisioned notification token from objectd through a fixed one-shot capability-transfer path.” It retains the absence of actual resource lease, object/mapping allocation, child creation, dynamic task, clone, fork, pthread, teardown, W^X, ELF loading, `dpkg` and `apt` claims.
