# Phase 36: root-constructed child control-lease M2 gate

## Status

**Status: verified as a separate dual-gated QEMU TCG proof.** On x86_64/PC99 image `fefe10fb3f5def365076060d54380933dfbf5c8b7daad0a0fbc006db2b34b7c5`, built with `SeLinRootConstructM1Adapter=ON` and `SeLinRootConstructM2ControlLease=ON`, `verify_root_construct_m2.py` SHA-binds the M1 provenance adapter, M2 root lease dispatcher, root wiring/main, taskd client, inert child, both protocols, CMake gate, image and fresh serial trace. This gate extends only the separately verified M1 profile. It does not change the default M0 production image or Phase 27 dynamic-child M2 gate.

> M2 is a provenance proof, not a task-start or dynamic lifecycle feature. It may transfer one control handle for the exact child that root constructed in M1, but taskd must not invoke the handle and the child must remain suspended.

## Closed transaction

After the exact M1 construction transaction has reached `ROOT_CONSTRUCTED`, the same isolated taskd adapter client may issue exactly `(LEASE_TCB, slot=1, generation=1)`. Root validates normal IPC with the closed three-word shape, sets one cap in the reply from the root-retained TCB of the M1-created child, and replies `(LEASE_GRANTED, slot=1, generation=1)` with exactly one extra capability. The client installs the received cap only in its fixed receive slot, validates the reply, makes no `seL4_TCB_*` invocation, and sends one duplicate request that root rejects without another cap.

| Participant | M2 allowed behavior | Explicitly excluded |
|---|---|---|
| Root | Retains M1 child provenance and transfers exactly one cap copy after `ROOT_CONSTRUCTED`. | A second child construction, a second cap transfer, child start/resume/suspend/configuration, cleanup, revoke, delete or reuse. |
| Taskd adapter client | Sets one fixed cap-receive path, validates one received cap, then observes duplicate rejection. | TCB operation, CNode authority outside the fixed receive path, VKA/VSpace/root-CNode access, child start, capability re-transfer. |
| Child | Remains suspended under the root-selected M1 image. | Any execution, completion, Linux ABI, VFS, device/KAPI or nested task creation. |
| Probe | None; the taskd adapter client is the only endpoint holder. | Child-cap receipt or client-selected parameters. |

## Mandatory controls

The new M2 opcode must be distinct from M1 construction. The endpoint is still sent only to the one taskd adapter client. No request contains an image pointer, stack/register/TLS value, clone flag, task ID beyond the fixed slot, scheduler parameter, credentials, signal, VFS or device field. Root must retain a root-local child provenance record from successful M1 construction; a missing/failed construction, malformed request or duplicate lease request replies `REJECTED` with zero extra caps and no allocation. The root must clear the outgoing cap field on every no-cap reply path.

The independent verifier must SHA-bind the opt-in M2 image, root adapter, root wiring/main, M2 protocol, taskd client, inert child, CMake gate and fresh QEMU log. It must prove that only root has `sel4utils_configure_process`, only the taskd client configures the fixed cap receive path, and neither taskd client nor child contains `seL4_TCB_*`, allocator/VSpace, retype, device, IRQ, DMA, PCI, I/O, IOSpace or Linux clone code. It must prove M1 construction precedes the one granted lease and the duplicate lease is rejected without a cap.

## Promotion rule

The verified M2 may claim only one opaque child-TCB control-cap transfer whose provenance is one exact M1 root construction, with the child still suspended and the receiving taskd client proving non-exercise; it also proves the duplicate lease is rejected with zero extra caps. It cannot claim a restricted TCB-rights model, child start, scheduler control, dynamic task allocation, clone/fork/pthread, teardown/reuse, process exit/wait/signals/futex, native ELF loading, W^X, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_root_construct_m2.verification.json`, checked by `./tools/verify_root_construct_m2.py` as part of the final 43-verifier regression suite.
