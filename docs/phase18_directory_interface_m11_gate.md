# SeLinOS Phase 18 directory interface M11 gate

**Status:** design gate only. SeLinOS has no directory object, directory FD, pathname traversal, per-FD cursor or `getdents64` implementation. The three immutable records and one volatile record are fixed server projections, not a directory tree.

A safe next step may prove only an **explicit rejection** of `getdents64` for every current FD with a fixed Linux-compatible `-ENOTDIR` result, or a separately designed literal root-handle experiment. It must not fabricate directory entries, return a success byte count, imply `/` traversal, allocate dynamic FDs, create a cursor, expose metadata, or claim compatibility with `readdir`, libc directory APIs, `find`, package extraction or `dpkg`.

| Required invariant | M11 restriction |
|---|---|
| FD input | Current fixed record FDs only; all other values reject. |
| Buffer input | No root mapping/copy on rejection path. |
| Output | Exact negative error return only; no serialized `linux_dirent64` record. |
| Server IPC | No new ROMFS directory opcode and no CPIO directory interpretation. |
| Package boundary | No package/`apt`/`dpkg` claim; persistent filesystem and runtime blockers remain unchanged. |

> **Decision:** a successful directory-record proof would be misleading at the current architecture stage. M11 may proceed only as a no-I/O, no-IPC, fail-closed ABI rejection gate until a real directory/object model exists.
