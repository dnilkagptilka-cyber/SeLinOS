# Phase 34 checkpoint: root-held fixed child bundle adapter

## Current state

Phase 34 remains **design-only**. The production image is the independently verified Phase 33 M2 image:

```text
5eb11722c82b9ccf980ff6e0b467295302458ce88431bc1213b5a8d42c29095e
```

The image verifies the closed trace in which taskd receives one fresh root-preprovisioned child TCB control cap, performs one `seL4_TCB_Resume`, and validates one fixed completion. It does **not** construct a child at runtime.

## Blocking implementation fact

The existing construction helpers are root-local: `sel4utils_configure_process`, VKA allocation and loader-VSpace operations run during bootstrap in `domain_manager.c`. After bootstrap, root runs the Linux ABI fault-mediated path and idle loop; it has no separately designed, cap-bounded request endpoint or serialized construction transaction that can safely accept a taskd request while retaining root's allocator and loader-VSpace authority.

Adding a synchronous root-construction request path without a dedicated gate would conflate Linux-fault mediation, bootstrap capability state and new runtime construction authority. It would be an unverified authority expansion and is therefore prohibited.

## Required next prerequisite

A later Phase 34 implementation must first define and prove a root-construction request service with all of the following properties:

| Requirement | Needed evidence |
|---|---|
| Dedicated endpoint and exact request grammar | Taskd cannot invoke or influence any existing ABI-fault, VFS or device endpoint. |
| Serialized one-slot transaction | At most one request; all request fields are constant slot/generation values. |
| Root-only construction authority | Only root uses VKA, VSpace and process-loader calls. |
| Exact state accounting | A root-held `FREE → ROOT_CONSTRUCTED` state record prevents duplicate construction. |
| Fault-safe failure result | Any construction failure returns a fixed fail-closed status and cannot leak partial handles. |
| No capability delivery in the first proof | Construction and capability lease are distinct future increments. |

Until that prerequisite is independently verified, **dynamic child construction, task-object allocation, clone, fork, pthread, task teardown/reuse, package execution, `dpkg` and `apt` remain blocked**.

## Root runtime-loop finding

`src/projects/helixos/src/main.c` calls `selinos_domain_manager_start()`, lowers the root priority to zero, then enters an unconditional `seL4_Yield()` loop. Therefore, adding only an endpoint to `domain_manager.c` cannot produce a runtime construction adapter: there is no root receiver after bootstrap. A valid implementation must separately specify the root receive-loop dispatch ordering, priority and starvation behavior, and must ensure that the construction endpoint cannot be confused with any existing unknown-syscall fault endpoint. This is a design finding, not a runtime construction claim.

## Persistent construction-context prerequisite

`selinos_domain_manager_start()` currently creates `simple_t`, `vka_t` and `vspace_t` as function-local bootstrap variables, then returns to `main.c`. Even if their backing allocations remain physically present, Phase 34 may not rely on their post-return lifetime or expose them through implicit globals. Before root can handle a runtime construction request, a separately reviewed root construction context must define ownership, initialization, shutdown policy and access serialization for the allocator and loader VSpace. This is an additional prerequisite; no runtime construction implementation has been attempted.

## Bootstrap-storage finding

`domain_manager.c` already uses static `allocator_static_pool` and `root_vspace_data`, but `simple_t`, `vka_t` and `vspace_t` are currently created as local descriptors in `selinos_domain_manager_start()`. Static backing storage alone is insufficient evidence that the descriptors may be used after the function returns. Phase 34 must move or reinitialize the complete root-owned construction context under an explicit static-lifetime contract before installing a post-bootstrap request loop.
