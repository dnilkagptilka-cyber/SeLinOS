# SeLinOS Phase 19 TLS `arch_prctl` gate

**Status:** implementation path discovered; not yet a compatibility claim.

The previous TLS blocker was accurately scoped to the fault reply frame: x86_64 `seL4_Fault_UnknownSyscall` message registers do not contain an FS-base field, so root cannot emulate `ARCH_SET_FS` by editing only the reply frame. The generated pinned seL4 client API, however, exports a distinct privileged TCB invocation:

```c
seL4_Error seL4_TCB_SetTLSBase(seL4_TCB tcb, seL4_Word tls_base);
```

Root retains `probe.thread.tcb.cptr` while it fault-mediates the isolated Linux probe. Therefore a possible bounded M12 contract is `arch_prctl(ARCH_SET_FS, base)` where root validates `base` as an already mapped single probe page, calls `seL4_TCB_SetTLSBase(probe.thread.tcb.cptr, base)`, requires `seL4_NoError`, then replies `0` and lets the original TCB resume. The isolated probe must prove a subsequent `%fs:` load/store reaches only its own mapped TLS page.

| Required condition | Scope boundary |
|---|---|
| Syscall | Exact x86_64 `arch_prctl` number `158`, only operation `ARCH_SET_FS = 0x1002`. |
| Base | Exactly one currently mapped anonymous test page; no arbitrary canonical addresses or kernel-reserved range. |
| Authority | Root invokes its retained target-TCB cap only. The TCB capability is never copied to the Linux probe. |
| Proof | `%fs:` store/load of a fixed word after the mediated call; distinct QEMU success marker and source/evidence verifier. |
| Exclusions | `ARCH_GET_FS`, GS-base, arbitrary TLS layouts, clone inheritance, ELF TLS relocation, dynamic linker and glibc compatibility remain unclaimed. |

> This path would remove the narrow reply-frame blocker only. It does not provide a Linux process/runtime, a dynamic linker or package installation capability.
