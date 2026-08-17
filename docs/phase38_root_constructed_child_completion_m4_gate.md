# Phase 38: root-constructed child completion witness M4 gate

## Status

**Status: verified as a separate four-gated QEMU TCG proof.** On x86_64/PC99 image `d58477f9e213763000c7b7f215b308f62a6e65f1f046ff51cffeaec642f97f5d`, built with `SeLinRootConstructM1Adapter=ON`, `SeLinRootConstructM2ControlLease=ON`, `SeLinRootConstructM3SingleResume=ON` and `SeLinRootConstructM4Completion=ON`, `verify_root_construct_m4.py` SHA-binds the root M1/M2/M4 sources, root wiring/main, M4 client/child, protocols, CMake gate, image and fresh serial trace. M4 depends on the separately verified M1 construction, M2 provenance cap transfer and M3 single-resume profiles. It is not an implementation of Phase 27 dynamic-child M2 and does not alter default M0, M1, M2 or M3 images.

> M4 may prove one fixed child-to-taskd completion message after the one M3 resume. It does not establish a general IPC namespace, child exit semantics, wait, signal delivery, futex join, process supervision or Linux ABI.

## Closed topology and trace

Before root spawns the compile-time M4 witness child suspended, root allocates one completion endpoint. It copies its send cap into the M4 child’s fixed CSpace slot and its receive cap into the M4 taskd client’s fixed CSpace slot. The root retains the original cap and does not transfer allocator authority. Taskd then runs the closed M1 construction and duplicate-refusal sequence, receives the M2 TCB cap in an M4-specific fixed slot, validates the M2 duplicate zero-cap refusal, and executes the one M3 resume. The child emits exactly `(CHILD_DONE, slot=1, generation=1)` on its endpoint. Taskd receives and validates this one normal message before emitting its M4 success marker.

| Participant | M4 allowed behavior | Explicitly excluded |
|---|---|---|
| Root | Allocates one endpoint and copies only its send/receive caps into the exact M4 child/taskd CSpace slots before M1 spawn. | A second endpoint, arbitrary endpoint routing, non-root allocator delegation, child start/resume/control, cleanup, revoke or reuse. |
| Taskd M4 client | Receives the fixed completion endpoint at setup, receives the one M2 TCB cap in its M4 control slot, resumes once, receives and validates one fixed completion. | A second resume, generic receive loop, endpoint/cap transfer, child configuration, allocator/VSpace/device authority, wait/exit semantics. |
| M4 child | Prints an execution witness, sends the exact one completion record, then yields. | Additional IPC, arbitrary messages, completion retries, filesystem, Linux ABI, device/KAPI, nested creation or exit semantics. |

## Mandatory controls

M4 is a default-OFF CMake profile requiring M1, M2 and M3. The M4 endpoint slots, TCB receive slot, message length, opcode, slot and generation are compile-time constants. No root IPC request names an endpoint, image, stack, register, TLS, clone flag, scheduler field, credential, signal, VFS or device field. The root must copy exactly the single completion cap to each intended M4 process and retain all VKA/loader-VSpace/endpoint allocation authority.

The independent verifier must SHA-bind M4 image, M1/M2/M4 root sources, M4 taskd client/child, all protocols, CMake gate and fresh QEMU log. It must prove that root alone allocates the completion endpoint; the M4 taskd client contains exactly one `seL4_TCB_Resume`, one exact receive and no other TCB action; the M4 child contains one exact `seL4_Send` and no other IPC; and only the precise root-selected slots are used. Ordered markers must show M1 construction, M2 cap grant/rejection, M3 resume, M4 child execution/completion and taskd completion validation.

## Promotion rule

The verified M4 may claim only one fixed child-to-taskd completion witness after one provenance-bound resume. It cannot claim dynamic IPC, a general process lifecycle, clone/fork/pthread, wait/exit/signal/futex semantics, FD/TLS/credential inheritance, teardown/reuse, ELF dynamic linking, W^X, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_root_construct_m4.verification.json`, checked by `./tools/verify_root_construct_m4.py` as part of the final 45-verifier regression suite.
