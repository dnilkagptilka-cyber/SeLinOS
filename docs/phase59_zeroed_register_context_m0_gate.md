# Phase 59: zeroed initial register-context M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinTaskdZeroedContextProbe=ON` profile started from the Phase 58 topology: a generation-2 target TCB with an assigned x86_64 PML4, self-rooted target CSpace, mapped ordinary IPC-buffer frame at `0x70000000`, and no execution history. Root constructed a 20-word zero `seL4_UserContext`, wrote all fields with `seL4_TCB_WriteRegisters(..., resume_target=0u, arch_flags=0u, count=20u, ...)`, then read all 20 fields back with `seL4_TCB_ReadRegisters(..., suspend_source=0u, arch_flags=0u, count=20u, ...)`. The requested input was all zero; pinned seL4 x86 sanitization normalized only read-back word 2 (`rflags`) to `0x202` by enforcing the architectural bit 1 and interrupt-enable bit. The other 19 read-back words were zero. The SHA-bound QEMU TCG transcript, independent verifier, 31-profile rebuild, and 65-standalone-verifier regression passed.

> A whole-context zero request with `resume_target=0u` and kernel-normalized `rflags=0x202` is a configuration witness, not a runnable-thread or Linux-process claim.

## Required transaction

1. Reproduce the Phase 58 generation-1 rollback and generation-2 TCB/CNode/PML4/notification/frame construction.
2. Populate target CSpace slots 0–2 with self-CNode, notification, and IPC-buffer frame capabilities.
3. Assign the target PML4 ASID, map the ordinary frame at the fixed IPC-buffer address, and configure the still-suspended TCB.
4. Write exactly one all-zero `seL4_UserContext` with complete x86_64 context count and `resume_target=0u`.
5. Read back the complete context without suspending or invoking the target; fail closed unless words other than `rflags` are zero and `rflags` exactly equals kernel-normalized `0x202`.
6. Move only final TCB/CNode/PML4 capabilities to taskd. A status-only service/probe pair must witness one owned transaction then one rejection.

## Forbidden scope

The implementation must not invoke `seL4_TCB_Resume`, use a true `resume_target` flag, set an entry point, stack pointer, `rflags`, FS/GS base, priority or scheduling configuration, invoke target IPC, load/mapping an ELF image, or execute code in the target. It must make no Linux thread/process, ABI, driver, `dpkg`, or `apt` claim.

## References

[1]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API Reference"
[2]: https://docs.sel4.systems/Tutorials/libraries-1.html "seL4 Libraries: Initialisation & Threading"
