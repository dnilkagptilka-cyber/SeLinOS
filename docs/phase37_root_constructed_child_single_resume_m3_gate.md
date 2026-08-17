# Phase 37: root-constructed child single-resume M3 gate

## Status

**Status: verified as a separate triple-gated QEMU TCG proof.** On x86_64/PC99 image `eef120983f9b1ed4d48ee4faa6def55d8e8df9e6beb09be14f39c6792b287cfd`, built with `SeLinRootConstructM1Adapter=ON`, `SeLinRootConstructM2ControlLease=ON` and `SeLinRootConstructM3SingleResume=ON`, `verify_root_construct_m3.py` SHA-binds the M1/M2 root sources, root wiring/main, M3 client, witness child, protocols, CMake gate, image and fresh serial trace. M3 depends on the separately verified construction M1 and provenance control-lease M2 proof. It does not alter the default M0 profile, the M1 status-only proof, the M2 non-exercise proof, or the Phase 27 dynamic-child gate.

> M3 may prove one execution of one root-selected child after taskd invokes exactly one `seL4_TCB_Resume` on the M2-received capability. It is not Linux `clone`, fork, pthread creation, a scheduler interface or a general child lifecycle.

## Closed scenario

The M3 taskd client repeats the fixed M1 construction transaction and its duplicate refusal, receives exactly one M2 provenance TCB cap in its already fixed CSpace slot, and validates the M2 duplicate rejection. It then calls exactly once `seL4_TCB_Resume(fixed_slot)` and requires `seL4_NoError`. The M3-specific root-selected child image emits one execution-witness marker and only yields thereafter. The QEMU trace must order construction, M2 grant, M2 duplicate refusal, taskd resume marker and child witness.

| Participant | M3 allowed behavior | Explicitly excluded |
|---|---|---|
| Root | Constructs the predeclared M3 witness child suspended and provides M2 provenance as already specified. | Any `seL4_TCB_*` control, additional child, extra cap, child configuration after M1, scheduler operation, teardown or reuse. |
| Taskd M3 client | Performs exactly one `seL4_TCB_Resume` on the received slot after all M1/M2 validation. | Suspend, second resume, set priority/affinity/registers/TLS, CNode transfer, allocator/VSpace/device authority, child configuration or cleanup. |
| M3 child | Prints one execution witness then yields. | Completion protocol, IPC, filesystem, Linux ABI, device/KAPI, nested child creation, exit semantics or further lifecycle action. |

## Mandatory controls

M3 must be a default-OFF CMake profile requiring both M1 and M2 gates. The root chooses the M3 client and child image at compile time; no IPC request selects an image, stack, register, TLS, clone flag, scheduler parameter, credential, signal, VFS or device field. The root must preserve M2's single cap grant and zero-cap duplicate rejection. M3 cannot receive another control cap or use any slot other than the one fixed M2 receive slot.

The independent verifier must SHA-bind the M3 image, root M1/M2 sources, M3 client and child, both existing protocols, M3 fixed-action protocol, CMake gate and a fresh QEMU trace. It must check that exactly the M3 client contains one `seL4_TCB_Resume`, that root has no `seL4_TCB_*` control, that M3 client has no other TCB APIs, and that the child lacks TCB/allocator/VSpace/device/Linux code. Ordered trace markers must show one construction, one cap grant, one duplicate refusal, one resume and one child witness; any failure marker or a duplicate-control marker invalidates the proof.

## Promotion rule

The verified M3 may claim only one successful direct resume by taskd of one root-constructed, M2-provenance child, together with one execution-witness marker. It cannot claim lifecycle generality, task creation API, clone/fork/pthread, a process ABI, wait/exit/signal/futex behavior, credentials, namespace/FD inheritance, teardown/reuse, ELF dynamic linking, W^X, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_root_construct_m3.verification.json`, checked by `./tools/verify_root_construct_m3.py` as part of the final 44-verifier regression suite.
