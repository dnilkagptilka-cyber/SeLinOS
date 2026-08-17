# SeLinOS Phase 12 Linux-to-volatile-VFS bridge M1 gate

**Status:** design gate. The verified VFS M0 service offers only a fixed server-owned volatile record. Linux ABI M6 still exposes only the immutable `/selinos-release` bridge. This document constrains a future M7 bridge that may connect a native Linux syscall probe to `/selinos-state` without turning either result into a general Linux VFS claim.

| Linux syscall | Admissible M7 request | Root action | Explicitly forbidden |
|---|---|---|---|
| `openat` | `AT_FDCWD`, exactly the NUL-terminated literal `/selinos-state`, zero flags/mode. | Copy at most 15 pathname bytes from one mapped user page into a root-local buffer; compare every byte; issue only the fixed ROMFS `OPEN_PATH` IPC. | Arbitrary pointers, path traversal, directories, flags, permissions, mounts or inherited FDs. |
| `read` | Fixed volatile FD `5`, exactly eight bytes, mapped one-page destination. | Invoke fixed VFS `READ`; copy exactly one returned word through a temporary root mapping; restore the reply frame. | Streaming, offsets other than zero, partial/multi-page reads or shared mappings. |
| `write` | Fixed volatile FD `5`, exactly eight bytes, one mapped user-page payload. | Copy exactly eight bytes into a root-local word; issue fixed VFS `WRITE`; return only the server result. | Arbitrary fd/write size, partial append, write-through storage, block I/O or persistence. |
| `close` | Fixed FD `5`. | Invoke fixed VFS `CLOSE`; reject later use in the same probe. | General descriptor tables, fork/exec inheritance or cleanup-on-thread-exit semantics. |

> **Stop rule:** M7 must not use a device mapping, descriptor ring, IRQ, DMA capability or backing block request. It must retain the separate VFS M0 non-persistence claim and add no `dpkg`, `apt`, POSIX filesystem, TLS, clone or blocking-futex claim.

## Required safety and evidence

A future implementation must use a purpose-named bounded user-string copy helper, rather than treating the generic console-write copier as a pathname interface. The helper must enforce a single mapped page, maximum length, NUL termination, exact byte equality and temporary root-only mapping teardown. The payload copy must similarly use a fixed eight-byte length and must not retain a client mapping or pointer.

M7 requires a new isolated probe, a new independent evidence record and a QEMU verifier that checks initial state, malformed-path rejection, malformed-write non-mutation, successful one-word replacement, read-back and post-close rejection. The record must declare that the word is reset at boot and that no persistence or hardware storage authority exists.
