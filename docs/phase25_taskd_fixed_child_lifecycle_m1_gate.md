# Phase 25: taskd-owned fixed-child lifecycle M1 gate

## Status and purpose

**Status: verified on default x86_64/PC99 QEMU TCG image `031013d68448c80f962b6c8b784cd5c473da00530c1358bdb8ab14b158a8123d`.** M15 verifies one fixed immutable `fcntl(F_GETFL)` request; it does not reduce the task-lifecycle blocker. M1 is now the verified task-management foundation: an isolated `taskd` domain, rather than the client and without device authority, controls the start, completion observation, suspension and resumption of one pre-provisioned child TCB.

> This is **not** `clone(2)`, `fork(2)`, pthread creation, general process management or dynamic child construction. Root is allowed to construct the fixed fixture during bootstrap. The claim begins only after the named child-control capability has been transferred to `taskd` and is exercised there.

| Property | M1 required behavior | Explicitly excluded |
|---|---|---|
| Child identity | One fixed bootstrap-created child, identified only by taskd-local ID `1`. | Dynamic PID allocation, parent/child hierarchy or lookup by arbitrary PID. |
| Authority | `taskd` owns the child TCB control cap and only the dedicated start/completion IPC caps. | Untyped memory, root CSpace, child VSpace, device frames, IRQ, DMA, PCI or scheduler-control authority. |
| Client protocol | A dedicated lifecycle probe sends one exact `START(1)` request over a taskd endpoint. | A Linux syscall ABI branch, a public task-management API or arbitrary IPC messages. |
| Child activation | taskd issues the first child-start notification, receives an exact completion message, suspends then resumes the child, issues one second child-start notification, then observes the second exact completion. | User-chosen entry point, stack, register image, address space or scheduling parameters. |
| Evidence | QEMU TCG log proves taskd request acceptance, two child completions, taskd TCB suspend/resume and final probe success. | Hardware scheduler behavior, SMP, priority inheritance, signals, wait, exit, futex join or resource reclamation. |

## Capability topology

The root creates three isolated process domains with their normal separate TCB, CSpace and VSpace construction: `selinos-taskd`, `selinos-task-lifecycle-child`, and `selinos-task-lifecycle-probe`. It then creates exactly four dedicated IPC objects: one client-to-taskd endpoint, one child-to-taskd completion endpoint, one taskd-to-child start notification, and one taskd-to-probe success notification. No object is reused from ROMFS, driver, VFS or Linux syscall mediation.

After bootstrap construction, root copies the child TCB cap and only the relevant fixed endpoint caps into `taskd`'s CSpace. The lifecycle probe receives only the client endpoint and one taskd-to-probe success notification. The child receives only its start-wait capability and its one completion-send capability. Root retains its bootstrap references solely for construction and diagnostic failure reporting; the M1 functional path must not invoke a root lifecycle operation after taskd has started.

| Domain | Must receive | Must not receive |
|---|---|---|
| `taskd` | Fixed child TCB cap; taskd client endpoint; child start/completion caps; probe-success notification cap. | Untyped, root CNode, child VSpace, client TCB, any device/IRQ/DMA/IOSpace capability. |
| Lifecycle child | Start wait cap; completion-send cap. | taskd endpoint, client endpoint, TCB caps, allocator objects and device authority. |
| Lifecycle probe | Client-to-taskd endpoint and one taskd-success notification. | Child start/completion caps, child TCB cap, allocator objects and device authority. |
| Root | Construction-time references and normal fault endpoints. | Participation in M1 activation, completion or TCB control after setup. |

## Pinned M1 protocol

The client request is exactly two machine words: `SELINOS_TASKD_M1_START` and task ID `1`. It is one-way: taskd cannot defer an endpoint reply while receiving child completions, so final client success is communicated solely through the separate taskd-to-probe notification. Any other label, message length, opcode or task ID is rejected without touching the child TCB. On the exact request, taskd sends the child the first start token. The child emits completion record `(SELINOS_TASKD_M1_CHILD_DONE, 1, generation)`; generation is fixed to `1` for the first start and `2` only after the one taskd-controlled suspend/resume cycle followed by taskd's explicit second start token.

Taskd must validate each completion record exactly. Between the two generations it invokes `seL4_TCB_Suspend` and `seL4_TCB_Resume` using the fixed child TCB cap, then sends the one explicit second start token. The second completion cannot be credited before both the resume and second-start operations. The lifecycle probe receives only its dedicated success notification after both exact completions are observed. Any malformed request or completion, unexpected badge, seL4 error, second client request or missing completion is fail-closed and emits no M1 success marker.

## Safety boundaries

M1 introduces **no Linux `clone` syscall** and makes no claim about child register construction. The child image, stack and entry point are static build artifacts selected by root at boot; taskd may only exercise lifecycle control over the single capability that root explicitly delegated. In particular, this gate prohibits sharing or duplicating file tables, address spaces, TLS bases, credentials, signal state, namespaces, futex state, scheduler parameters or VFS handles.

The child must be deliberately inert outside the M1 activation protocol. It may issue only the completion IPC and then wait for the next taskd start token. It must not use Linux ABI faults, ROMFS, KAPI, device servers or direct console output as evidence. The taskd server and lifecycle probe use stable marker strings only after validation of all previous transitions.

## Required implementation slices

| Slice | Required change | Fail-closed condition |
|---|---|---|
| Build wiring | Add dedicated `selinos-task-lifecycle-child` and `selinos-task-lifecycle-probe` images; do not repurpose generic `servers/service.c`. | Missing images, wrong CPIO inclusion or non-isolated process configuration. |
| Protocol header | Add a taskd-private protocol header with fixed opcodes, IDs, record widths and marker constants. | Any message label/length/value outside the declared protocol. |
| Root wiring | Configure the three fixed processes and two new IPC objects; copy only the declared caps to each CSpace slot. | Slot mismatch, cap-copy error, unexpected cap or root-mediated runtime transition. |
| taskd service | Replace only the `taskd` placeholder with the fixed M1 state machine. | Any transition outside `idle → first-done → suspended/resumed → second-done → complete`. |
| Child/probe | Implement one static wait/complete child and one exact client request/response probe. | Any nonzero status, wrong generation or unexpected extra message. |
| Evidence | Add an independent verifier, SHA-bound QEMU log and capability-source inspection. | Missing marker, root fallback marker, malformed-message acceptance or scope-inflating claim. |

## Evidence contract

A successful QEMU TCG proof must contain all of the following ordered facts: taskd accepts exact `START(1)`; child completion generation 1 is validated; taskd suspends then resumes the declared child TCB and issues its one second start token; child completion generation 2 is validated; the lifecycle probe receives success. The verifier must bind the image, root wiring source, taskd implementation, child implementation, probe implementation, protocol header and runtime log by SHA-256.

The verifier must additionally inspect source-level negative controls: no `SELINOS_LINUX_CLONE` dispatcher constant or branch; no `vspace_new_pages`, `sel4utils_configure_process`, `seL4_Untyped_Retype`, device-memory, IRQ, DMA or PCI operation in taskd; and no root function call that activates, suspends or resumes the child after the initial cap handoff. This is a bounded architecture proof, not a substitute for dynamic capability auditing.

## Promotion criteria

M1 is verified: `verify_taskd_fixed_child_lifecycle_m1.py` passed against the SHA-bound QEMU log `selinos_taskd_fixed_child_lifecycle_m1.boot.log`, and the complete default-plus-opt-in suite passed with 33 independent verifiers. The compatibility matrix states only: “one taskd-owned lifecycle proof for a fixed pre-provisioned child TCB.” The clone/task gate and every excluded semantic above remain in force.

The next possible lifecycle step would be an authority-reviewed taskd allocator/CSpace/VSpace design. That later step must establish dynamic construction, stack/register policy, cleanup and fault ownership before any Linux `clone` request shape is considered.
