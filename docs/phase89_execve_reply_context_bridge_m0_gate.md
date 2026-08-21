# Phase 89 Gate — controlled `execve` reply-context bridge M0

**Status:** VERIFIED — the separate x86_64/PC99 QEMU TCG Phase 89 profile reached `mov rax,[rsp]; ud2` after root changed the faulted target's fixed `RIP`/`RSP` through `TCB_WriteRegisters` and issued one 16-word reply through `FaultIP`; its independent SHA-bound verifier passed. Cross-profile regression, commit, and publication remain pending.

> **Boundary:** Phase 89 would prove one self-authored transition consisting of an authenticated Linux x86_64 `execve` number-59 UnknownSyscall fault, one root-owned whole-context read/write that changes only `RIP` and `RSP`, one label-zero 16-word reply that restores `RAX` through `FaultIP` but excludes reply-frame `SP` and `FLAGS`, and one `mov rax,[rsp]; ud2` terminal witness with `RAX=1`.

| Boundary | Candidate proof | Explicit exclusion |
|---|---|---|
| Fault input | One fixed syscall 59, badge, and zero `RDI`/`RSI`/`RDX` request | General `execve`, arbitrary pointers, caller-selected programs or process model |
| Context authority | Root reads then writes the fixed target's complete context and changes only fixed `RIP`/`RSP`; all other words must round-trip unchanged | General debugger, arbitrary TCB control, scheduler policy or a user-visible context API |
| Reply | Exactly one label-zero 16-word fault reply spans `RAX` through `FaultIP`; it deliberately excludes reply-frame `SP` and `FLAGS` | A normal 18-word reply-frame restoration proof or a claim that seL4 reply-frame `SP` handling is correct |
| Witness | RX `mov rax,[rsp]; ud2` reads fixed `argc=1`; root receives terminal `UserException` at fixed post-read IP and reads `RAX=1` | `argv`, `envp`, auxv traversal, initial-stack compatibility, dynamic linker or normal process execution |
| Memory | Root-private aliases are removed before RX and RW+NX target mappings | W+X memory, host files, host interpreter, Linux kernel, VM, container or compatibility mode |

## Non-claims

Phase 89 will not claim a normal 18-word UnknownSyscall reply-frame handoff, general `execve`, `fork`, `clone`, process lifecycle, ELF loading, `PT_INTERP` resolution, `ld-linux`, dynamic linking, glibc, musl, Linux ABI compatibility, Debian binary execution, `dpkg`, or `apt`.

## Publication gate

Before publication, a production-clean isolated profile, deterministic QEMU TCG transcript, SHA-bound manifest, independent verifier, all-profile rebuild, evidence-binding audit, and complete standalone verifier regression must pass. The normal 18-word Phase 88 reply-frame failure must remain documented rather than being concealed by this distinct bridge mechanism.
