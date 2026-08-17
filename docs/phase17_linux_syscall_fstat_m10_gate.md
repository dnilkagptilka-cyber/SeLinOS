# SeLinOS Phase 17 Linux syscall ABI M10 `fstat` gate

**Status:** design gate only. No `fstat` implementation or compatibility claim exists yet.

M10 may expose only a probe-local, explicitly documented record for one already-open fixed immutable FD. It must not claim the Linux libc `struct stat` ABI, host-kernel layout compatibility, arbitrary file descriptors, directories, metadata persistence, device nodes, timestamps, permissions, ownership, symlinks or POSIX filesystem semantics.

| Proposed constraint | Required proof |
|---|---|
| Accepted FD | Only a literal immutable SeLinOS record FD already opened by the same isolated probe. |
| User buffer | One page already mapped by the current anonymous test mapping; exact bounded offsets checked before every root copy. |
| Record layout | A SeLinOS-declared probe record with a pinned byte layout, not an unqualified `struct stat` claim. |
| Values | Fixed type/size fields derived only from the already-proven immutable record. |
| Failure | All other FDs, unaligned/invalid buffers and unsupported forms return an exact declared negative result without VFS IPC side effects. |

> The long-term apt/dpkg gate still requires a general filesystem and process runtime. M10 can at most verify one syscall-shape prerequisite; it cannot establish package compatibility.
