# Phase 88 Gate — controlled `execve` reply initial-stack read M0

**Status:** REPLY-FRAME BLOCKED; DIRECT-CONTEXT WITNESS OBSERVED — the normal 18-word unknown-syscall reply-frame variant failed, while a temporary diagnostic using root `TCB_WriteRegisters` followed by an empty fault reply reached `mov rax,[rsp]; ud2` and observed `RAX=1`. This proves the mapped stack and witness are viable but does not verify the normal reply-frame contract. A second temporary bridge diagnostic, which explicitly restored `RIP`/`RSP` with `TCB_WriteRegisters` and then issued a 16-word reply through `FaultIP` (excluding reply-frame `SP` and `FLAGS`), also reached `mov rax,[rsp]; ud2`; it is a distinct reply-context bridge candidate, not a normal 18-word reply-frame proof. Temporary diagnostic output showed the initial `UnknownSyscall` fault correctly reported `SP=0x70002f80`, the root IPC buffer still held reply `SP=0x70002f80` immediately before `seL4_Reply`, and a root `TCB_ReadRegisters` after the second fault still observed target `RSP=0x70002f80`. The second fault is a **data** `VMFault` (prefetch flag `0`) at interpreter entry `0x60004000` with VMFault address `0`, FSR `4`, and `RAX=0`. The generated seL4 x86_64 fault-register table confirms that the 18-word `UnknownSyscall` reply contains `RSP`, and the pinned kernel’s `handleFaultReply` calls `copyMRsFaultReply` for `MIN(length, n_syscallMessage)` when reply label is zero. The unresolved problem is therefore runtime handoff behavior, not omission of the `SP` reply slot or an intentionally rejected fault reply. No verifier, compatibility claim, commit, or publication exists.

> **Purpose:** Phase 88 isolates the single blocker recorded by Phase 87: whether a fixed x86_64 unknown-syscall reply can redirect execution to a self-authored RX witness which then reads the first eight-byte word at the reply-supplied `RSP` and reaches a terminal `UD2` instruction without a VM fault.

## Fixed contract

| Boundary | Required M0 proof | Excluded from this milestone |
|---|---|---|
| Trigger | One fixed Linux x86_64 `execve` syscall-number 59 unknown-syscall fault from one self-authored fixture | General `execve`, user-selected syscall arguments, filesystems or a process model |
| Reply | Root restores exactly the x86_64 18-word syscall reply frame, changes `FaultIP` to one fixed RX witness, and supplies one fixed aligned `RSP` | General register/context preservation or arbitrary reply values |
| Stack data | The mapped RW+NX target stack contains only one fixed first word, `argc = 1`; the witness executes exactly `mov rax, [rsp]` before terminal `UD2` | `argv`, environment, auxv traversal, pointer dereference, writable target access or a complete Linux initial stack |
| Observation | Root receives exactly one terminal invalid-opcode `UserException` at a fixed post-read instruction address and observes the expected `RAX = 1` value | Fault reply, repair, second resume, stack mutation, normal interpreter return or program execution |
| Memory safety | Root-private stack/witness aliases are removed before target mappings; stack is RW+NX and witness is RX | W+X pages, arbitrary mappings, host execution, host linker or host filesystem access |

## Required evidence

The milestone must provide an isolated x86_64/PC99 QEMU TCG build, an ordered transcript, a SHA-bound manifest, and an independent verifier. The transcript must establish the authenticated syscall fault, the one complete reply, and the post-read terminal user exception with the expected return register. The verifier must reject a VM fault, a repeated unknown syscall, a wrong terminal IP/vector, any second resume, any reply after the terminal exception, or an unexpected stack word.

## Explicit non-claims

Phase 88 will not claim general `execve`, `fork`, `clone`, a Linux process model, full initial-stack construction, `argv`, `envp`, auxv correctness, `PT_INTERP` resolution, `ld-linux`, glibc, musl, dynamic linking, ELF loading, Linux ABI compatibility, Debian binary execution, `dpkg`, `apt`, a Linux kernel, a Linux VM, a container, chroot, or compatibility mode.

## Publication gate

No status may advance beyond **REPLY-FRAME BLOCKED; DIRECT-CONTEXT WITNESS OBSERVED** until the normal 18-word unknown-syscall reply-frame variant has a successful isolated runtime witness, independent verifier, all-profile rebuild, evidence-binding audit, and complete standalone verifier regression pass. A Phase 88 commit must be pushed to the private repository only after those gates complete.
