# Phase 87 Gate — controlled native `execve` replacement M0

**Status:** VERIFIED — clean isolated x86_64/PC99 QEMU TCG proof establishes one fixed execve-number fault transition to a stack-independent interpreter terminal witness. Complete cross-profile regression and publication remain pending.

> **Recorded blocker:** Phase 87 does not prove preservation or restoration of the Phase 86 initial stack/auxv across its unknown-syscall replacement reply. The milestone’s runtime witness deliberately performs no stack read after the replacement entry. Native executable startup remains blocked on a separately verified context-and-stack handoff contract.

## Purpose

Phase 87 is the first narrowly bounded process-model increment after the verified controlled `ET_DYN` relative relocation (Phase 85) and controlled `PT_INTERP` handoff (Phase 86). It will prove a single **native SeLinOS process replacement transaction** whose Linux-visible entry is a fixed x86_64 `execve` syscall number. The transaction must not return to the old user context: it replaces one root-owned, suspended target context with one fixed self-authored ET_DYN-plus-interpreter handoff profile.

> This is an `execve`-shaped non-returning replacement proof for exactly one SeLinOS-owned process slot. It is not general `execve`, fork, clone, vfork, a Linux process model, a filesystem launcher, glibc execution, Debian binary execution, `dpkg`, or `apt`.

| Boundary | Fixed Phase 87 contract | Rejected or excluded |
|---|---|---|
| Request | One `execve` syscall-number fault from one fixed process control endpoint with fixed pathname/argv/envp representation | Arbitrary user pointers, multi-process callers, filesystem traversal, symlinks, shebangs, `execveat`, `fexecve` or path search |
| Authority | Root-owned immutable executable descriptor selects exactly one embedded Phase-86-class fixture and interpreter witness | Host executable, host linker, host filesystem lookup, Linux kernel, VM, container, chroot or compatibility mode |
| Atomicity | Validate all fixed request fields and fixture metadata before retiring the old target context; no target resume after partial failure | Partial mapping, caller-selected caps/frames, recovery by returning a synthetic success or source-context restart |
| Context | Retire one prior target context, construct exactly one replacement VSpace/TCB context, then resume replacement once at interpreter entry | `fork`, `clone`, vfork, threading, PID allocation policy, FD inheritance, signals, wait/reap, credentials or namespaces |
| Result | Old context never resumes; reply redirects to one stack-independent interpreter `UD2` terminal witness | Any claim that the inherited initial stack, `argc` or auxv remains readable after reply; general dynamic linker, shared libraries, symbol resolution, TLS, constructors or process-exit semantics |

## Required proof obligations

1. The native syscall gateway authenticates Linux x86_64 `execve` number **59** and only accepts the exact fixed request shape; every other process-control syscall is rejected deterministically.
2. The control plane owns the selected image descriptor; raw user pathname/argv/envp pointers are observed only under an explicit bounded mapping rule and cannot direct a host or root filesystem lookup.
3. Construction uses root-private aliases and unmaps them before RX/RW+NX target mappings. No target page is W+X.
4. The old entry cannot resume after the replacement decision. The same fixed TCB fault-reply context is redirected to a self-authored interpreter terminal witness; this is not yet a fresh-VSpace or stack-preserving process replacement proof.
5. A QEMU TCG transcript, SHA-bound manifest and independent verifier establish the ordered authenticated fault/reply/terminal-interpreter sequence. All-profile rebuild, evidence refresh, zero binding mismatches and full regression remain publication gates.

## Explicit non-claims

Phase 87 will not claim a general `execve` implementation, general ELF loading, `PT_INTERP` resolution, `ld-linux`, glibc or musl, `DT_NEEDED`, shared libraries, dynamic TLS, constructors, ordinary Linux process lifetime, exit status, fork/clone/vfork/pthread, PIDs, signals, wait/reap, credential/namespace behavior, file-descriptor inheritance, VFS execution, Debian package execution, `dpkg`, `apt`, a Linux kernel, a Linux VM, a container, chroot or compatibility mode.

## Relation to Debian 13.6 acceptance

A native Debian process requires much more than this gate: general ELF/filesystem loading, standard initial stack and auxv, normal dynamic linker/libraries, Linux syscall and signal behavior, process lifecycle, file-descriptor semantics, durable VFS and user-space services. Phase 87 exists only to turn the next process-replacement gap into a small reproducible proof.

## References

[1]: https://man7.org/linux/man-pages/man2/execve.2.html "Linux execve(2) manual page"
[2]: https://man7.org/linux/man-pages/man2/syscalls.2.html "Linux syscalls(2) manual page"
[3]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
