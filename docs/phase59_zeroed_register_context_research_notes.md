# Phase 59: zeroed initial register-context M0 — research notes

## Pinned local ABI observations

The project’s pinned x86_64 `seL4_UserContext` holds, in order, `rip`, `rsp`, `rflags`, general-purpose registers `rax` through `r15`, then `fs_base` and `gs_base`: **20 machine words** in total. The generated local client stub defines:

```c
seL4_TCB_WriteRegisters(tcb, resume_target, arch_flags, count, regs)
```

The x86_64 `resume_target` bit is explicit. The Phase 59 root invocation must pass `0u`, `arch_flags` must be `0u`, and `count` must cover the entire 20-word context. A zero-initialized `seL4_UserContext` is therefore a precise all-register-zero request, including `rip`, `rsp`, `rflags`, FS base, and GS base.

## Gate constraints

The Phase 58 target already has a distinct CSpace, PML4, ASID, mapped IPC-buffer frame, and `seL4_TCB_Configure` association. Phase 59 may call `seL4_TCB_WriteRegisters` exactly once with the zero context and `resume_target=0u`, followed by `seL4_TCB_ReadRegisters` for the whole context to prove exact read-back. It must not call `seL4_TCB_Resume`, assign an entry point or stack, load ELF, invoke target IPC, or establish a Linux-process claim.

> A zeroed and read-back register context in a suspended TCB is not an execution witness: a zero `rip`/`rsp` deliberately provides no runnable program setup.

## Kernel-normalized RFLAGS finding

The first QEMU proof rejected an all-zero read-back. Pinned `src/kernel/src/arch/x86/machine/registerset.c` explains why: `sanitiseRegister()` forces `FLAGS_HIGH` (bit 1) and `FLAGS_IF` (bit 9), clears reserved low bits and trap flag, then masks unsupported bits. The corresponding pinned x86 definition sets `FLAGS_USER_DEFAULT` to `FLAGS_IF | FLAGS_HIGH`, i.e. **`0x202`**. Phase 59 therefore verifies a requested all-zero context, with exactly one kernel-normalized read-back exception: word 2, `rflags`, must equal `0x202`; the other 19 ABI words must equal zero.

## Sources

[1]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API Reference"
[2]: https://docs.sel4.systems/Tutorials/libraries-1.html "seL4 Libraries: Initialisation & Threading"
[3]: `src/kernel/libsel4/sel4_arch_include/x86_64/sel4/sel4_arch/types.h` — pinned local `seL4_UserContext`
[4]: `build-taskd-ipc-buffer-probe/libsel4/include/interfaces/sel4_client.h` — generated local TCB register client stubs
[5]: `src/kernel/src/arch/x86/machine/registerset.c` and `src/kernel/include/arch/x86/arch/machine/cpu_registers.h` — pinned x86 RFLAGS sanitization policy
