# Phase 32: root-preprovisioned child-TCB control lease M1 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `158e95b60b922622021f2408f996c359b4dec03cbe5b604ab3c11a1cde4606a6`.** Phase 31 verifies a one-shot opaque notification-token transfer. The next possible authority increment is a one-shot transfer of a **root-preprovisioned child TCB control capability** from a dedicated objectd lease service to a dedicated taskd lease service. This is not TCB allocation, child setup, start, resume, clone or dynamic process construction.

> The child TCB must already exist as a root-constructed inert process fixture, and it must remain suspended before, during and after the M1 proof. The transferable control cap is a bounded lease token; it must not be exercised in M1.

## Required lifecycle boundary

Root constructs one dedicated inert child image with separate TCB, CSpace, VSpace and fault endpoint. Root retains its construction references. It then delegates one copy of the child TCB control cap to the dedicated objectd-TCB-lease service. Objectd returns exactly that cap to taskd through a fixed one-cap reply path. Taskd receives it into one fixed otherwise-empty CSpace slot, validates only the reply shape, signals success and remains prohibited from calling `seL4_TCB_Suspend`, `seL4_TCB_Resume`, `seL4_TCB_Configure`, `seL4_TCB_WriteRegisters`, `seL4_TCB_SetTLSBase` or any scheduler operation on it.

| Actor | Exact M1 authority | Must not do |
|---|---|---|
| Root | Preconstructs the inert child, creates endpoints/notification, delegates one TCB-cap copy to objectd and keeps bootstrap references. | Participate in the post-transfer control path, start the child or describe a dynamic task allocator. |
| Objectd TCB lease service | Holds one exact child TCB cap in a pinned source slot and transfers it once to taskd. | Allocate/retype a TCB, create CSpace/VSpace, configure/start child, transfer other cap or perform device/memory operation. |
| Taskd TCB lease service | Receives one cap at a pinned destination slot and emits a success notification. | Exercise TCB control, forward cap, allocate anything, configure/start child, modify VSpace/CSpace or provide Linux `clone`. |
| Inert child | Remains suspended and has no protocol cap. | Run, signal completion, receive resources or inherit taskd/client state. |
| Probe | Sends one fixed request then waits for success. | Receive, name, use or control child cap. |

## Required proof controls

The root wiring must name the exact child fixture and verify objectd’s source slot and taskd’s receiver destination slot. Objectd must return exactly one extra cap. Taskd must use a fixed CNode receive path and exact extra-cap/message checks. A fresh QEMU log must contain ordered client acceptance, objectd transfer, taskd receipt and probe success markers, as well as a marker that the dedicated child remains suspended. The independent verifier must reject all TCB invocation strings from taskd/objectd and any allocator, VSpace, device, IRQ, DMA, PCI, I/O, IOSpace or clone operation in the new services.

## Promotion rule

M1 is verified: `verify_tcb_control_lease_m1.py` passed against the SHA-bound QEMU log `selinos_tcb_control_lease_m1.boot.log`, and the complete default-plus-opt-in suite passed with 38 independent verifiers. The compatibility matrix claims only: “taskd received a one-shot control-cap copy for one root-preprovisioned suspended child TCB and did not exercise it.” It cannot claim dynamic object creation, stack/register setup, VSpace/CSpace construction, child start, exit, cleanup, cap revocation, reuse, fork, pthread, `clone`, scheduler semantics, W^X, ELF loading, `dpkg` or `apt`.
