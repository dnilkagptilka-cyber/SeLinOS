# Phase 34: root construction-dispatch contract

## Status

**Verified only for the fixed M1 opt-in proof.** On the `SeLinRootConstructM1Adapter=ON` x86_64/PC99 QEMU TCG image `ecd60208f7dba0aa34a817aad06d256685835307c90cb78eafd31ccf6c378fb5`, root blocks only on its dedicated construction endpoint after bootstrap, accepts one closed request, constructs one fixed child bundle and then rejects the duplicate. `verify_root_construct_m1.py` binds that proof. This is not a general root dispatcher and does not alter the default M0 production profile.

## Endpoint separation

The construction request endpoint is created by root and copied only into the dedicated taskd adapter process. It is neither the Linux ABI probe's fault endpoint nor an endpoint shared with ROMFS, driver, objectd, memd, consoled or generic service domains. Root must retain the sole receive cap.

| Endpoint class | Receiver | Message class | M1 treatment |
|---|---|---|---|
| Unknown-syscall fault endpoint | Root during the closed ABI probe | `seL4_Fault_UnknownSyscall` | Remains inside `run_linux_syscall_abi_probe`; never dispatched by construction service. |
| Root construction endpoint | Root runtime dispatch loop | Normal IPC with exact 3-word request | One exact `REQUEST(slot=1,generation=1)` accepted at most once. |
| Any other root-visible message | None | Undefined | No badge-based reinterpretation; fails closed by absence of a copied send cap. |

## Serialized transaction

After all existing bootstrap proofs have completed, the M1 root runs a loop that blocks only on the construction endpoint. The loop maintains a root-local state word `FREE`, `ROOT_CONSTRUCTING`, `ROOT_CONSTRUCTED` or `REJECTED`. It accepts only the exact three-word normal IPC request tag, fixed slot and fixed generation from `FREE`. The QEMU proof sends one valid request followed by one duplicate: the duplicate receives the fixed `REJECTED` record and does not call an allocator, loader or child-control function. The implementation also rejects malformed requests fail-closed.

A construction attempt enters `ROOT_CONSTRUCTING`; either it completes into `ROOT_CONSTRUCTED` with an opaque status reply, or it transitions to `REJECTED` with no child-handle delivery. M1 does not restore `FREE`: rollback, cleanup and reuse are not claimed.

## Liveness and priority

Root lowers its priority after the construction client is started and then executes a blocking receive on the dedicated endpoint. That policy is QEMU-proven. It does not wait on fault endpoints or poll arbitrary CSpace slots. The default M0 profile instead retains its separately verified one-shot status transaction followed by its idle/yield loop.

## Required M1 negative checks

The verifier must prove the construction endpoint is not passed to the ABI probe or any existing server; root accepts no badge alternatives; request words do not include caller-chosen image, stack, register, TLS, clone, scheduler, credential, VFS or device fields; and construction failure does not leak child control caps. `taskd`, its probe, objectd and memd must not acquire VKA, loader VSpace or root CNode authority.
