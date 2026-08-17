# SeLinOS dpkg/apt prerequisite gate

**Status:** planning gate only. SeLinOS does **not** run `dpkg`, `apt`, Debian packages or package maintainer scripts. This document records the necessary evidence sequence before such a claim can be made.

Debian packages contain executables, libraries, configuration and documentation; installation requires archive unpacking, dependency state and package lifecycle handling.[1] The documented `dpkg --install` flow includes control-file extraction, maintainer scripts, filesystem unpack/backup and package configuration.[2] APT adds repository acquisition, architecture handling, compressed indexes, dependency resolution and time/security-related metadata checks.[3]

| Prerequisite | Present SeLinOS evidence | Missing work before `dpkg`/`apt` claim |
|---|---|---|
| Read-only boot filesystem | Bounded CPIO `newc` parser, two-file archive, fixed and packed path lookup. | Arbitrary bounded paths, directory tree, symlinks, metadata, multi-page/streaming reads and per-client FD ownership. |
| Writable POSIX-like filesystem | VFS M0 proves one server-owned volatile full-word replacement through fixed IPC, with malformed-write non-mutation and close rejection. M7 additionally bridges only exact Linux `/selinos-state` open/read/fixed-word-write/read-back/close. | Persistent block backend, filesystem server, atomic rename/replace, durable writes, locks, ownership and recovery tests. Both M0 and M7 state are reset/lost and are not POSIX-like persistence. |
| Debian archive handling | None. | `.deb` ar/container parsing, compression support, tar extraction, control-file parsing, path safety and package-content validation. |
| dpkg state database | None. | Transactional package status/info database, locks, rollback/recovery and trigger state. |
| Process and script execution | `getpid`, basic identity/TID returns, bounded mmap/brk and fixed ROMFS bridge only. | TLS (`ARCH_SET_FS`), clone/futex/task lifecycle, signal/process management, full syscall surface, dynamic linker and executable W^X-safe mapping. |
| Networking for APT acquisition | Root-only PCI BAR proof exists; no NIC resource delegation. | NIC domain, DMA containment, IP/DNS/TCP, HTTP(S), time, trust roots and repository signature verification. |
| Security/update policy | Parser hash-bound evidence only. | APT Release/InRelease verification, keyring policy, clock validation, downgrade/replay policy and audit trail. |

> **Gate rule:** no SeLinOS milestone may state “runs dpkg”, “runs apt” or “supports Debian/Linux packages” until it contains an independent end-to-end verifier for a specific pinned package-manager build and profile, including persisted package state and declared trust/network conditions.

The nearest verified stepping stones are the packed `OPEN_PATH` ROMFS protocol, VFS M0 and the fixed Linux-to-volatile-VFS M7 bridge. `OPEN_PATH` accepts one 1–16-byte printable pathname in four IPC words and performs no user-pointer mapping. VFS M0 adds a single fixed `/selinos-state` record, owned by the isolated server, with a complete one-word IPC replacement; the state is explicitly volatile and has no block or durability backend. All three are intentionally insufficient for Debian’s general filesystem and package lifecycle needs.

## References

[1]: https://www.debian.org/doc/manuals/debian-faq/pkg-basics.en.html "Debian FAQ: Basics of the Debian package management system"
[2]: https://www.man7.org/linux/man-pages/man1/dpkg.1.html "Linux manual page: dpkg(1)"
[3]: https://manpages.ubuntu.com/manpages/focal/man5/apt.conf.5.html "APT configuration file manual"
