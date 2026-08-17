# Phase 27 taskd dynamic fixed-child M2 checkpoint

## Current implementation state

M2 remains **unimplemented by design**. The verified production system contains only Phase 25 M1: taskd controls a single root-preprovisioned inert child. The current objectd and memd services are placeholders and do not expose finite one-shot object or mapping leases. Root alone presently owns the `vka_t` allocator and loader `vspace_t` required by `sel4utils_configure_process`.

Implementing M2 by invoking those root-held helpers from taskd would give taskd broad allocation and loader-VSpace authority, contradicting the capability boundaries established in Phase 26. Implementing M2 as a root-created second static child would only duplicate M1 and would not establish a dynamic lifecycle claim. Neither shortcut is permitted.

## Required first implementation increment

The next code change must be a standalone **objectd M1 finite inventory proof** or **memd M1 fixed mapping inventory proof**, not a `clone` branch. It must introduce one server-owned, fixed-slot lease interface with no client-controlled object, mapping or device parameter; prove exact grant/refusal behavior in QEMU; and add an independent verifier before taskd consumes a lease.

| Candidate prerequisite | Safe bounded claim if verified | Must remain excluded |
|---|---|---|
| objectd M1 | Root-configured objectd reports the availability and one-shot reservation of a named non-device task-slot inventory. | TCB/CSpace construction by client/taskd, arbitrary retype, untyped delegation, dynamic tasks or clone. |
| memd M1 | Root-configured memd reports the availability and one-shot reservation of a named fixed mapping plan. | General VSpace allocation, arbitrary frame mapping, executable loading, W^X claim or dynamic tasks. |

No M2 code, evidence record, image digest or compatibility claim is authorized until at least one prerequisite is independently verified and its authority transfer is reviewed against Phase 26 and Phase 27.
