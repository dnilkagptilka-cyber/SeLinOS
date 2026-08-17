# Phase 26: taskd dynamic-authority prerequisite gate

## Status

**Status: design gate only.** Phase 25 M1 proves taskd’s control over a single root-preprovisioned child TCB. It does not grant taskd object-creation, CSpace construction, VSpace construction, register-image, stack, cleanup or identifier authority. Those missing capabilities make any Linux `clone(2)` claim unsafe and are therefore the next critical-path design problem.

> **Decision:** dynamic child construction must be decomposed into objectd, memd and taskd leases. No server may receive a general root allocator capability, and taskd must never gain device, IRQ, DMA, PCI, IOSpace, page-table-global or arbitrary untyped authority merely to create a task.

## Target M2 boundary

The next lifecycle increment, provisionally **taskd M2**, may prove only that taskd can request and activate one fresh child from a finite root-defined task-slot budget. The client does not choose the executable, virtual address, stack pointer, initial registers, capability layout, priority, CPU affinity or credentials. The child executes a fixed inert lifecycle image and signals a capability-bound completion record. It is not exposed through a Linux syscall.

| Component | Bounded delegated role | Forbidden authority or semantics |
|---|---|---|
| objectd | Creates a finite configured set of kernel objects from a root-defined per-slot inventory and returns only per-child control caps on a one-shot lease. | Arbitrary untyped allocation, device objects, IRQ handlers, IO ports, IOSpaces, global CSpace manipulation or client-selected object types. |
| memd | Creates one fixed-size private VSpace and fixed read/write stack region for the declared inert child image, then returns only the child’s address-space control bundle to taskd. | Shared mappings, executable writable pages, arbitrary mappings, host files, user-selected addresses or hardware frames. |
| taskd | Validates a fixed `CREATE_FIXED_CHILD` request, assembles the named lease, configures the child’s fixed entry/stack/register image, starts it, observes its completion and requests deterministic teardown. | General `clone` flags, user entry points/stacks/TLS, sharing flags, file tables, credentials, signals, scheduler policy, device authority or reusable allocator power. |
| child | Runs the fixed image and reports a single completion token to taskd. | Linux syscall ABI, VFS, KAPI, direct allocator requests or child spawning. |
| client probe | Requests one fixed child and verifies only the pinned taskd success record. | Child control caps, object leases, VSpace caps or any Linux compatibility API. |

## Lease model

A task slot is a finite root-declared record with a monotonically increasing generation counter and the lifecycle states shown below. A capability from one generation must not be accepted as a capability for a later generation. There is no slot reuse claim until the exact teardown proof is independently verified.

| State | Owner of transition | Required proof condition |
|---|---|---|
| `FREE` | objectd/memd lease coordinators | No taskd or client handle references a live child object. |
| `RESERVED` | taskd after exact client request | The slot ID is fixed and the request has no client-controlled resource parameter. |
| `OBJECTS_READY` | objectd | TCB, CNode, fault endpoint and only declared fixed IPC objects were created from the slot inventory. |
| `VSPACE_READY` | memd | The child has the fixed image and stack mapping plan, with an auditable non-writable executable policy requirement held as a separate W^X blocker. |
| `CONFIGURED` | taskd | The fixed CSpace/VSpace, initial register image and fault endpoint were installed exactly once. |
| `RUNNING` | taskd | Child was resumed only after complete configuration and has no unexpected authority. |
| `COMPLETED` | taskd | Exact one-shot completion record matches slot and generation. |
| `TEARDOWN_PENDING` | taskd/objectd/memd | Child is suspended and no new client transition is accepted. |
| `FREE` | future separately verified teardown milestone | All child caps, mappings, IPC objects and accounting references have been revoked/deleted. |

## Non-negotiable gates

The M2 implementation may not start until the following design conditions have explicit source-level contracts and independent verifier checks.

| Gate | Required contract |
|---|---|
| Object inventory | A compile-time upper bound on child slots and every object type/size; no caller-controlled retype. |
| Capability provenance | Every taskd-received cap has a named source, slot, rights mask, generation and teardown owner. |
| Address-space policy | memd owns child mappings; taskd receives only the explicitly necessary address-space control cap. Native W^X/executable loading remains blocked until the separate x86 NX control evidence exists. |
| Initial state | Fixed entry point, fixed stack top, zeroed general-purpose register policy, fault endpoint and no inherited TLS/FD/VFS state. |
| Fault handling | A child fault transitions the slot to `TEARDOWN_PENDING`; it cannot be mistaken for normal completion. |
| Teardown | Suspension, unmapping, cap revocation, object deletion and lease-accounting proof are required before any claim of slot reuse. |
| Concurrency | M2 admits exactly one outstanding create request and one child. Multiple clients, racing creates and scheduling fairness remain excluded. |

## Required evidence for a later M2 claim

The independent verifier must bind the image, objectd, memd, taskd, child, probe, protocol header and QEMU log by SHA-256. It must prove a single ordered trace: exact request acceptance; objectd lease delivery; memd fixed mapping delivery; taskd one-time configuration; child start; exact completion; transition to teardown-pending. It must reject source paths containing Linux clone dispatch, client-controlled entry/stack fields, unbounded allocation loops, device/IRQ/DMA/PCI operations or direct allocator authority in taskd.

A later complete teardown milestone must separately prove cap deletion/revocation and VSpace/object release before it changes `TEARDOWN_PENDING` to `FREE`. Until then, M2 may make no slot-reuse, resource-reclamation, process-exit, fork, pthread, `clone`, `wait`, signal, futex, dynamic-linker, `dpkg` or `apt` claim.

## Relationship to existing blockers

The gate does not resolve the x86 W^X blocker: a static inert image may be used for structure proof, but no general native ELF or user-selected executable mapping follows. It does not resolve IOMMU, persistent VFS, network or package-management blockers. It is only the prerequisite authority model required before an honest Linux `clone` request can be designed.

## API feasibility finding

The current `sel4utils_configure_process` interface accepts a caller-held `vka_t` allocator and loader `vspace_t`; it creates the child CSpace/VSpace/fault endpoint and loads the ELF image through that caller-owned authority. Consequently, forwarding the existing helper to taskd would give taskd general allocator and loader-VSpace power, violating this gate. A valid M2 therefore requires explicit objectd and memd one-shot leases or a root-side construction adapter whose taskd-visible authority is limited to a named per-slot bundle. This finding is architectural evidence only; it is not a dynamic-task implementation or clone claim.

## Root-allocation inventory finding

The implementation inventory confirms that `vka_alloc_*`, `vspace_*` and `sel4utils_configure_process` calls are currently concentrated in `domain_manager.c`; objectd and memd are still isolated placeholders. This preserves the present authority boundary but means no runnable dynamic M2 implementation can truthfully claim objectd/memd lease delegation yet. The next implementation work is therefore the explicit lease-adapter protocol, not a taskd-side call to root helpers and not a `clone` syscall branch.
