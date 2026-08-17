# SeLinOS Phase 24 `fcntl(F_GETFL)` M15 gate

**Status:** design gate only. Current ROMFS and volatile VFS records use fixed FDs and do not implement a general descriptor table or mutable file-status flags.

M15 may accept only x86_64 `fcntl` syscall `72` with command `F_GETFL = 3` for a fixed opened record FD in the isolated ABI sequence. The return value must be a pinned policy value: `O_RDONLY = 0` for immutable records and `O_RDWR = 2` only for the explicitly server-owned volatile state FD. The request must have no VFS IPC, no state mutation and no user-pointer access.

> **Scope boundary:** No `F_SETFL`, locks, `FD_CLOEXEC`, duplication, nonblocking I/O, ownership, arbitrary descriptors, file-table sharing or POSIX descriptor semantics are implied. This cannot establish a package runtime, `dpkg` or `apt`.
