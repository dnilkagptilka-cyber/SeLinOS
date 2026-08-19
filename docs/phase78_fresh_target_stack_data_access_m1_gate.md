# Phase 78: fresh target stack data-access M1 gate

## Status

**Status: VERIFIED — BLOCKED DATA-VMFAULT BRANCH.** The isolated `SeLinFreshTargetStackDataAccessProbe=ON` QEMU TCG profile executed exactly one `mov rax,[rsp]` attempt and received the exact classified non-prefetch `VMFault` at initial `rsp=0x70002ff8`; it sent no reply, performed no repair and issued no second resume. All 52 profiles rebuilt, all evidence SHA-256 bindings were refreshed, and 85 standalone verifiers passed (excluding only the two documented host-dependent checks). The result is a verified blocker classification, not a successful target stack read. Phase 78 M0 proved only the helper-reported entry/stack mapping ledger while the fresh target remained suspended.

> M1 is a **single read classification**, not a process stack. It does not establish stack contents, zero-fill, return addresses, red-zone behavior, `argc`/`argv`/`envp`/auxv, ABI alignment beyond the inherited fixed context, calls, unwinding, signals, C runtime, ELF, or Linux compatibility.

## Pinned source and target state

Phase M1 inherits Phases 73–75 and Phase 78 M0. Before fresh target mapping, root materializes exactly six entry bytes through a temporary root-only alias:

```text
48 8B 04 24 0F 0B
mov rax, [rsp]
ud2
```

The temporary alias must be unmapped before the entry frame is mapped RX at `0x60000000`. Phase 78 M0 then maps the NX stack frame at `0x70002000` with zero new paging objects and maps the entry frame RX with one new final page table. The target context remains `rip=0x60000000`, `rsp=0x70002ff8`.

## One-resume classification transaction

| Step | Required action | Required boundary |
|---:|---|---|
| 1 | Verify fixed Phases 74–75 addresses and M1 constants; materialize only `mov rax,[rsp]; ud2` through a temporary root alias; unmap that alias. | No stack alias, stack write, payload beyond six bytes, or target execution yet. |
| 2 | Complete inherited Phase 78 M0 stack NX / entry RX mapping ledger. | Exactly 0 stack and 1 entry paging objects. |
| 3 | Call `seL4_TCB_Resume(fresh_tcb.cptr)` exactly once and receive one badged fault. | No second resume; exactly one receive. |
| 4a | **Positive path:** receive `UserException`, badge `0x74`, `FaultIP=0x60000004`, `SP=0x70002ff8`, vector 6. | `mov` completed one stack read; `ud2` is terminal; no reply. |
| 4b | **Blocked classification path:** receive `VMFault`, badge `0x74`, `IP=0x60000000`, `Addr=0x70002ff8`, `PrefetchFault=0`. | Target faulted on the specified data read; no reply, repair, alias mutation, second resume or continuation. |

The implementation must fail closed if neither exact class is received. A build profile can be promoted only after its evidence truthfully names the observed branch; the positive post-read `UserException` branch is the only branch that removes the immediate initial-stack translation blocker.

## Required evidence and verifier scope

The default-OFF profile requires a CMake option, protocol header, QEMU TCG transcript, SHA-bound evidence manifest and standalone verifier. The verifier must inspect only the Phase M1 guard and assert: six exact payload bytes, alias unmap before target entry map, one resume, one receive, no reply, no second resume, exact positive and blocked classification fields, and absence of ELF/process/syscall/package operations. Earlier Phase 76–78 verifiers must remain scoped to their own guards.

## Explicit non-claims

Even a positive M1 branch proves only that one `mov rax,[rsp]` completed before a terminal `ud2`. It does not prove arbitrary memory access, stack initialization, a write, a return, process startup, ELF, dynamic linking, Linux ABI, `dpkg`, `apt`, Debian package compatibility, or a Debian primary environment.
