# SeLinOS Phase 20 TLS `ARCH_GET_FS` M13 gate

**Status:** design gate only. M12 proves a single mapped-page `ARCH_SET_FS` transaction through root’s retained probe-TCB capability. It does not expose a query mechanism.

M13 may add only `arch_prctl(ARCH_GET_FS = 0x1003, user_word_pointer)` after that exact M12 success. Root must return the **locally tracked accepted M12 base**, not infer an FS value from an UnknownSyscall frame and not claim a general remote-TCB FS-base read API.

| Requirement | M13 boundary |
|---|---|
| State | One local `probe_tls_base` variable initialized invalid for each fresh isolated probe and set only after `seL4_TCB_SetTLSBase` succeeds. |
| Request | Syscall `158`, operation `0x1003`, one user pointer whose word fits an existing mapped probe page. |
| Result | Store exactly the tracked base to that word and return `0`. |
| Failure | Before M12 set, wrong operation, invalid user pointer or any untracked base state rejects fail-closed. |
| Exclusions | General cross-thread TLS introspection, GS, clone/fork, ELF TLS, dynamic linker and glibc remain out of scope. |

> This is a state-echo verification step, not general `ARCH_GET_FS` Linux ABI compatibility. The package-runtime blockers remain unchanged.
