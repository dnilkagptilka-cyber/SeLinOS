# Phase 33: transferred child-TCB single-resume M2 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `5eb11722c82b9ccf980ff6e0b467295302458ce88431bc1213b5a8d42c29095e`.** Phase 32 proves that taskd can receive a one-shot copy of a root-preprovisioned suspended child TCB cap without exercising it. M2 may prove only one next control transition: taskd performs one `seL4_TCB_Resume` using that received cap, and the dedicated child emits one exact completion token. This remains far below Linux process creation.

> **M2 is not `clone(2)`.** The child is root-configured from a fixed image before taskd begins; taskd receives no allocator, child CSpace/VSpace configuration power, register/stack/TLS setup power, credentials, file tables, namespaces or caller-selected execution state.

## Exact M2 trace

The M2 bundle may reuse no live Phase 32 instance; it must create a fresh dedicated taskd/objectd/child/probe quartet with a fresh child TCB. Root configures but does not spawn the child, gives objectd exactly one child-TCB cap and gives the child only a taskd-child completion endpoint. Objectd transfers the cap once. Taskd receives it into the named fixed CSpace slot, validates the reply, invokes exactly one `seL4_TCB_Resume` on that slot, then waits for exact completion `(CHILD_RAN, slot=1, generation=1)` and signals probe success. There must be no suspend, second resume, register write, TCB configure, TLS base set, priority change or scheduling-context operation.

| State | Owner | Allowed transition | Disallowed behavior |
|---|---|---|---|
| `SUSPENDED` | Root | Configuration completed; child not spawned. | Child execution, taskd control before valid cap transfer. |
| `TCB_RECEIVED` | Taskd | One extra-cap reply accepted at fixed destination slot. | Any cap forwarding, TCB setup, VSpace/CSpace change or TCB operation other than M2 resume. |
| `RUNNING_ONCE` | Taskd | One `seL4_TCB_Resume` on received slot. | Repeat resume, suspend, scheduler manipulation or any client-selected state. |
| `COMPLETED_ONCE` | Child → taskd | Exact fixed completion token. | Exit/cleanup/reuse claim, user syscall ABI, VFS/KAPI/device use. |
| `TERMINAL_PENDING` | Taskd | Signals probe success. | Teardown, release/revocation/reuse or another child lifecycle request. |

## Required negative controls

The independent verifier must bind all sources and runtime evidence. It must reject any `seL4_TCB_` invocation in objectd; in taskd, it must permit exactly the one lexical `seL4_TCB_Resume(SELINOS_..._TCB_DEST_SLOT)` and reject all other `seL4_TCB_` invocations. It must reject allocator, VSpace, retype, device, IRQ, DMA, PCI, I/O, IOSpace, Linux clone and user-provided stack/register/flags paths. The child must have only completion-send authority and no client, taskd-control, VFS, ABI or device cap.

## Promotion rule

M2 is verified: `verify_tcb_resume_m2.py` passed against the SHA-bound QEMU log `selinos_tcb_resume_m2.boot.log`, and the complete default-plus-opt-in suite passed with 39 independent verifiers. The compatibility matrix claims only: “taskd resumed exactly once a fresh root-preprovisioned child TCB after receiving its control cap, and observed one fixed completion.” It does not claim dynamic task creation, clone/fork/pthread, process exit, join/wait, futex exit, resource teardown/reuse, scheduler semantics, W^X, ELF loading, `dpkg` or `apt`.
