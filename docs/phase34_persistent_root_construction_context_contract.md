# Phase 34: persistent root construction-context contract

## Status

**Design-only.** The verified root bootstrap currently owns its allocator and loader VSpace inside `selinos_domain_manager_start()`. A runtime construction adapter requires a distinct persistent context; this contract must be implemented and independently proven before root serves construction requests.

## Context ownership

The persistent context is root-private and initialized exactly once before any construction adapter endpoint is made available. It contains only the root VKA, root loader VSpace, simple interface, one construction endpoint and one root-local transaction state. It is not copied to taskd, objectd, memd, child or probe CSpaces and no pointer or selector to it appears in IPC payloads.

| Context field | Owner | Runtime use | Not exposed to |
|---|---|---|---|
| Root VKA | Root | Fixed one-slot child construction only. | All non-root domains. |
| Root loader VSpace | Root | Load only one root-selected inert child image. | All non-root domains. |
| Construction endpoint receive cap | Root | Receive exact fixed construction request. | Root keeps receive cap; taskd gets only send cap. |
| Transaction state | Root | `FREE → CONSTRUCTING → CONSTRUCTED/REJECTED`. | No shared memory or status pointer. |
| Child construction records | Root | Provenance/accounting only. | No generic child handle or allocator surface. |

## Lifetime and serialization

The context must have static root lifetime, never reference a stack-local bootstrap descriptor after return and be initialized before root lowers its priority. There is exactly one root thread and no concurrent construction transaction. A second request after state leaves `FREE` obtains the fixed rejection response and cannot trigger new VKA/VSpace calls.

## Required implementation checks

The M1 verifier must bind the source of the static context declaration, the single initialization function, the root request handler and all child/probe artifacts. It must reject direct VKA/VSpace access outside root and reject code paths that use any caller-provided address, image, stack, register, TLS, clone, scheduler, credential, FD, device or mapping parameter. It must show that construction state is not reset and that no cap is delivered in M1.

## Non-claims

This contract does not authorize dynamic task creation yet. It does not prove construction transaction correctness, child startup, capability delivery, teardown, reuse, clone, fork, pthread, ELF W^X, package execution or Linux compatibility.

## Minimal migration plan

The refactor must introduce one root-private static structure containing `simple_t`, `vka_t`, `vspace_t` and an `initialized` bit. `selinos_domain_manager_start()` must initialize that structure exactly once and may use local aliases only after checking the bit. Existing PCI, ABI, ROMFS and service-start helpers continue receiving explicit `vka_t *`/`vspace_t *` arguments, so the refactor does not enlarge their authority or convert them into global consumers. The future construction handler receives references only from the root-private structure; no header exports mutable accessors.

The first proof after this refactor must establish only persistent context lifetime and unchanged bootstrap behavior. It must not expose a construction endpoint or create a post-bootstrap child. Dispatch and construction are later gates.

## Verification status

**Verified on default x86_64/PC99 QEMU TCG image `6c138a2370df4862e74cb05571dfd3c91597f324ec94dca664d04c6c453d1129`.** `verify_persistent_root_context_m0.py` binds the static root descriptor implementation and the fresh runtime log. The complete default-plus-opt-in regression suite passed with **40** independent verifiers.

The verified claim is limited to persistent root-private descriptor lifetime and one-shot bootstrap initialization. The construction request endpoint, root dispatch loop, runtime child construction, capability delivery and every Linux process-lifecycle claim remain unimplemented.
