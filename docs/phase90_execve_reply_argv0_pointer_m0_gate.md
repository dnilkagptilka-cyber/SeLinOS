# Phase 90 — Execve Reply `argv[0]` Pointer Read M0 Gate

**Status:** VERIFIED — isolated QEMU TCG witness, SHA-bound manifest, independent verifier, all-profile rebuild (`count=66`), zero-mismatch binding audit, and full standalone verifier regression (`count=98`) passed; Git publication is pending.

> **Boundary:** Phase 90 may prove only one self-authored x86_64 transition: an authenticated Linux syscall-number-59 `UnknownSyscall` fault, one root-owned reply-context bridge that changes only the fixed target `RIP` and `RSP`, one label-zero 16-word reply through `FaultIP`, and one RX `mov rax,[rsp+8]; ud2` witness. The terminal exception must occur at the fixed post-read address and root must read back exactly the self-authored `argv[0]` pointer from `RAX`.

| Boundary | Required evidence | Explicit exclusion |
|---|---|---|
| Fault input | One badged syscall-59 fault with fixed zero `RDI`, `RSI`, and `RDX` | General `execve`, pathname handling, caller-selected `argv`/`envp`, process creation, `fork`, or `clone` |
| Stack slot | Exactly the word at fixed initial-stack offset `+8` is read; it equals the fixed self-authored pointer to the in-page `selinos` byte string | `argc` revalidation, arbitrary stack reads, string dereference, `argv` traversal, `envp`, alignment proof, or ABI-complete initial stack |
| Context bridge | Root reads and writes the fixed target context, changing only fixed `RIP` and `RSP`; the reply carries only 16 words from `RAX` through `FaultIP` | A normal 18-word reply-frame `SP`/`FLAGS` restoration proof, debugger API, general TCB control, or scheduler semantics |
| Terminal witness | Terminal `UserException` follows the one pointer-slot load; root reads exact `RAX` from the fixed target context | Interpreter execution, `PT_INTERP` loading, ELF relocation, dynamic linker handoff, or executable Linux userspace |
| Memory authority | Target code is RX, target stack/data are RW+NX, and all root-private aliases are removed before execution | W+X mappings, Linux kernel, host filesystem, host interpreter, VM/container emulation, or compatibility mode |

## Non-claims

Phase 90 will not claim that the normal 18-word `UnknownSyscall` reply frame restores `RSP` correctly. It will not claim a general initial stack, complete `argv`, `envp`, auxv, `AT_*` semantics, stack-string dereference, `execve`, ELF loading, `PT_INTERP` resolution, `ld-linux`, dynamic linking, glibc, musl, native Linux ABI compatibility, Debian binary execution, `dpkg`, or `apt`.

The normal Phase 88 reply-frame result remains an unresolved negative finding: the standard 18-word reply path did not reach its terminal stack-read witness. Phase 90 must remain a separately named reply-context bridge proof and must not conceal or weaken that limitation.

## Publication gate

Before any publication claim, Phase 90 requires a production-clean isolated profile, deterministic QEMU TCG runtime transcript, SHA-bound manifest, independent verifier, all-profile rebuild, zero-mismatch evidence-binding audit, full standalone verifier regression, a private Git commit, and push to `phase63-reply-terminal-fault`.
