# Taskd M2 lease protocol draft

## Status

**Design-only.** This document defines the minimum IPC vocabulary that objectd, memd and taskd must implement before a bounded fresh-child proof can be attempted. It creates no executable code, capability transfer, task, VSpace, CSpace, stack or Linux ABI claim.

## Fixed request vocabulary

All requests use a closed one-slot namespace. The only admissible slot is `1`; no client supplies an object type, size, address, entry point, stack pointer, CSpace size, VSpace size, capability mask, priority, scheduling parameter or generation.

| Sender | Receiver | Opcode | Exact payload | Expected result |
|---|---|---|---|---|
| Lifecycle probe | taskd | `CREATE_FIXED_CHILD` | `(opcode, slot=1)` | A taskd-local pending result only; no resource cap returns to the probe. |
| taskd | objectd | `RESERVE_OBJECT_BUNDLE` | `(opcode, slot=1, generation=1)` | One immutable object-bundle lease or a fail-closed status. |
| taskd | memd | `RESERVE_FIXED_VSPACE` | `(opcode, slot=1, generation=1)` | One immutable mapping-bundle lease or a fail-closed status. |
| taskd | objectd/memd | `ABANDON_LEASE` | `(opcode, slot=1, generation=1)` | A pre-start cancellation acknowledgement only. |
| taskd | lifecycle child | `START_FIXED_IMAGE` | Notification only | The child may emit exactly one completion `(slot=1, generation=1)`. |

## Bundle requirements

The object bundle is an opaque capability package. Its contents must be fixed in the protocol definition and must never be selected by taskd or a client. M2 must define every member, recipient CSpace slot, rights mask, issuing server, revocation owner and consumption point. The mapping bundle follows the same rule; it must describe only a fixed static child image and fixed stack mapping plan. A mapping bundle cannot be used to establish a writable/executable mapping claim while the separate x86 W^X blocker remains unresolved.

| Bundle | May include | Must not include |
|---|---|---|
| Objectd object bundle | One fixed child TCB control cap, one child CNode cap, one child fault endpoint cap, fixed IPC caps needed by the M2 child image. | Untyped caps, CNode root authority, IRQ, device frame, IOPort, IOSpace, scheduler-control, arbitrary endpoint creation or any cap reusable outside `(slot=1,generation=1)`. |
| Memd mapping bundle | One fixed child address-space control cap plus the statically described image/stack configuration metadata. | General VSpace allocator, raw frame caps, existing client mappings, user-selected virtual addresses, arbitrary page rights or host-backed file mappings. |
| Taskd local state | Slot ID, generation, lifecycle state and source-verified lease identifiers. | Arbitrary process IDs, client-supplied Linux clone flags, task inheritance state, file descriptors, TLS or credentials. |

## Transfer and revocation rule

A lease-cap transfer must be one-shot. The issuing server retains the only revocation authority. If taskd sees an unexpected cap, opcode, message length, badge, generation or state transition, it must enter `TEARDOWN_PENDING`, issue no child start and request `ABANDON_LEASE`. The lifecycle probe cannot observe lease contents or acquire a child cap. Success does not imply teardown, reuse or a second request.

## Required tests before implementation

An implementation cannot start until a verifier plan can demonstrate all items below without inspecting untrusted runtime input.

| Test | Required result |
|---|---|
| Wrong slot | objectd, memd and taskd reject every slot except `1` without allocating or transferring anything. |
| Wrong generation | A generation mismatch cannot activate, consume or release a lease. |
| Duplicate request | The second `CREATE_FIXED_CHILD` request is rejected while slot `1` is non-`FREE`. |
| Partial lease | taskd does not configure or resume a child unless both opaque bundles match exactly. |
| Malformed completion | The child completion must not be treated as success unless `(slot=1,generation=1)` is exact. |
| Fault path | A child fault and a normal completion have disjoint handling; neither enables reuse. |
| Authority scan | taskd source contains no allocator, VSpace, retype, device, IRQ, DMA, PCI, IOPort or IOSpace operation. |

> The draft is deliberately narrower than Linux process creation. It is a capability-provenance prerequisite, not a process API, `clone(2)` contract or a substitute for normal userspace execution.
