# SeLinOS Phase 22 `clone` and task lifecycle gate

**Status:** design gate only. The current root bootstrap creates a fixed set of isolated seL4 process domains through `sel4utils_configure_process` and `sel4utils_spawn_process_v`. This proves startup isolation, not Linux `clone` semantics.

The fault-mediated Linux ABI probe is a single preconfigured process. Its documented contract now includes bounded TLS M12/M13, but still has no task manager, child scheduler model, dynamic CSpace/VSpace construction, stack preparation, register image policy, task IDs, parent/child TID handling, clear-child-tid, exit wake-up, wait, signal, file-descriptor table duplication or clone-flag validation.

| Required clone element | Current state |
|---|---|
| Clone syscall mediation | Not implemented. |
| Child TCB/CSpace/VSpace lifecycle | Not implemented under a taskd-owned policy. |
| Shared/private address-space flag semantics | Not implemented. |
| Child stack/register setup | Not implemented. |
| Parent/child TID and futex exit semantics | Not implemented. |
| Resource ownership and cleanup | Not implemented. |

> **Decision:** no `clone`, pthread, fork or process-lifecycle claim can be made until a task server owns an explicit capability lifecycle, applies a pinned subset of Linux flags and has an independent multi-domain functional verifier. A one-off root spawn is not an ABI substitute.
