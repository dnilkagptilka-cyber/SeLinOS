# Phase 34: root-held fixed-child bundle adapter M1 gate

## Status

**Status: verified as a separate opt-in QEMU TCG proof.** On x86_64/PC99 image `ecd60208f7dba0aa34a817aad06d256685835307c90cb78eafd31ccf6c378fb5`, built with `SeLinRootConstructM1Adapter=ON`, `verify_root_construct_m1.py` SHA-binds the root adapter, root wiring, root entry, taskd client, inert child, protocol, CMake gate, image and fresh serial trace. The current `sel4utils_configure_process` path requires the caller to hold both an allocator and loader VSpace. Delegating that helper to taskd, objectd or memd would violate the capability boundary established in Phase 26. The verified increment is therefore a **root-held, finite one-slot construction adapter** with a taskd-visible interface limited to named status.

> The adapter is not a general process constructor. It may materialize at most one fixed, root-selected inert child bundle from a compile-time inventory, exactly once, and it must retain all allocator and loader-VSpace authority in root.

## Required authority split

| Participant | M1 authority | Explicitly excluded |
|---|---|---|
| Root construction adapter | Holds the existing VKA/loader VSpace; constructs one fixed child CSpace, VSpace, fault endpoint, stack and fixed inert image. | Arbitrary client request parameters, multiple slots, client-supplied images, device authority delegation, unbounded allocation or reuse. |
| Taskd | Sends one exact `REQUEST_FIXED_BUNDLE(slot=1, generation=1)` request and receives only fixed status plus predeclared child-control handles after construction. | Calling `sel4utils_configure_process`, VKA/VSpace access, object-type selection, initial-register/stack/TLS control, Linux clone interface. |
| Objectd/memd | Remain evidence-bound status/lease coordinators; future typed object/mapping roles must be independently proven. | Becoming disguised root allocators in M1. |
| Child | Fixed root-selected inert image; begins suspended and is not started by adapter M1. | User-selected ELF, Linux ABI, VFS, device/KAPI or nested task creation. |
| Probe | Sends one fixed request and validates the exact adapter status. | Cap receipt, child control, object/mapping authority. |

## Exact M1 trace

The taskd-side request is a closed three-word message: `(REQUEST_FIXED_BUNDLE, slot=1, generation=1)`. Root validates the exact normal IPC shape, performs at most one child construction using its existing owned authorities, records state `FREE → ROOT_CONSTRUCTING → ROOT_CONSTRUCTED`, and returns a fixed status record to taskd through a dedicated adapter endpoint. The child is spawned suspended. The same client then receives one fixed duplicate rejection; neither transaction carries a capability. M1 does **not** transfer a TCB, CSpace, VSpace, fault endpoint or mapping cap; capability transfer will be a later separately gated increment once the construction provenance record is verified.

## Mandatory controls

The construction inventory must name every permitted object and mapping: exactly one TCB, one CNode, one fault endpoint, one fixed IPC-buffer frame, one fixed stack frame and the fixed inert image segments required by the existing loader. The request must contain no image pointer, stack pointer, register value, clone flag, sharing bit, PID/TID, priority, CPU affinity, credential, signal or file-table field. A second request is refused. No delete, revoke, unmap or reuse claim is allowed.

The independent verifier must bind root source, adapter protocol, taskd adapter client, child image fixture, probe and QEMU log by SHA-256. It must show that only root contains allocator/loader calls and reject allocator/VSpace calls in taskd and probe. It must reject Linux clone, `seL4_TCB_*` in the taskd adapter client, device/IRQ/DMA/PCI/I/O/IOSpace paths, arbitrary command dispatch and client-controlled setup fields.

## Promotion rule

The verified M1 may claim only that root constructed one fixed child bundle after an exact taskd-mediated request while retaining all construction authority and leaving the child suspended; it also proves a duplicate is rejected without another allocation. It cannot claim dynamic task allocation, child capability lease, task start, teardown, reuse, Linux `clone`, fork, pthread, process exit, wait, futex, native ELF loading, W^X, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_root_construct_m1.verification.json`, checked by `./tools/verify_root_construct_m1.py` as part of the final 42-verifier regression suite.

## Root-dispatch prerequisite

In the verified opt-in M1 profile, root starts the dedicated taskd adapter client, lowers its priority, and blocks only on the dedicated construction endpoint. The endpoint remains separate from the unknown-syscall fault endpoint used by the bounded ABI probe. The adapter uses exact normal IPC shape validation and fail-closed replies for malformed, duplicate and construction-failure cases; it does not multiplex endpoint messages by badge conventions. The default M0 profile remains separately scoped to its status-only transaction followed by an idle/yield loop.
