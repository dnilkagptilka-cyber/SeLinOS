# SeLinOS ROMFS CPIO M1: bounded multi-file pathname foundation

**Status:** verified on x86_64/PC99 QEMU TCG instruction-count mode. The isolated `selinos-romfsd` now links an immutable, reproducibly generated `newc` CPIO archive and resolves two fixed internal pathnames. This is a small VFS foundation; it is **not** a general Linux VFS, general `openat()` implementation or package-manager filesystem.

## Archive format and parser policy

The embedded archive uses Linux’s `newc` CPIO representation: a 110-byte ASCII-hex header, pathname including terminating NUL, and four-byte alignment before file data and between archive members.[1] The generator emits regular files `/selinos-release` containing `SELINOS!` and `/selinos-banner` containing `CPIO-OK!`, followed by `TRAILER!!!`.

The parser is linked only into the `selinos-romfsd` executable. It validates the `070701` magic, parses `c_mode`, `c_filesize` and `c_namesize`, bounds every header/name/data calculation against immutable archive length, enforces a 16-member scan limit, verifies NUL-terminated names and accepts only regular-file entries. Server startup performs a self-check that both named files resolve, have their expected length and produce their expected IPC payload word.

| Property | Verified M1 behavior | Not implemented |
|---|---|---|
| Archive representation | Embedded, uncompressed `newc` CPIO with `TRAILER!!!` | Compression, concatenated archives, dynamic archive loading or storage-backed archives. |
| Pathname resolution | Server resolves two fixed internal pathnames: `/selinos-release`, `/selinos-banner` | Client-supplied arbitrary paths, traversal, directory semantics or symlink resolution. |
| File policy | Exact expected-size regular files only | Metadata exposure, permissions, ownership, hard links, directories, device nodes or special files. |
| Read protocol | One offset-zero word payload per bounded file projection | Streaming/multi-page reads, arbitrary lengths/offsets, per-client file-position state or concurrent descriptor ownership. |
| Isolation | Archive is linked into `romfsd`; clients receive only endpoint capability slot 8 | Archive page mapping or parser authority in client/driver domains. |

> The existing `openat(AT_FDCWD, "/selinos-release") → read → close` Linux syscall proof remains unchanged. The root gateway still accepts only that constrained release route; the second CPIO file is verified by the direct isolated ROMFS probe and does not broaden the application ABI.

## Protocol projection and evidence

IPC client input remains deliberately bounded to file selector IDs. `romfsd` maps those selectors to fixed archive paths internally, returning FD 3 for the release record and FD 4 for the banner record. This avoids passing an untrusted pointer or unbounded pathname through the current minimal endpoint protocol while still proving server-side pathname lookup against real CPIO data.

The direct probe validates:

| Sequence | Expected result |
|---|---|
| Open/read/close release projection | Original FD 3 and `SELINOS!` behavior preserved. |
| Read invalid FD 99 | `EBADF`. |
| Open/read/close banner projection | FD 4 and `CPIO-OK!` returned from the embedded archive. |
| Root syscall route | Existing gateway completes `getpid` through `exit(0)`, including release `openat/read/close`. |

The runtime record [`tests/artifacts/selinos_romfs_cpio_m1.boot.log`](../tests/artifacts/selinos_romfs_cpio_m1.boot.log) contains both `embedded newc CPIO multi-file parser ready` and `CPIO pathname multi-file lookup passed`. The independent verifier [`tools/verify_romfs_cpio_m1.py`](../tools/verify_romfs_cpio_m1.py) pins the production image, parser, probe, protocol, generated archive, generator and runtime record by SHA-256.

```bash
cd /home/ubuntu/helixos
./tools/verify_romfs_cpio_m1.py
```

## Remaining work

The next VFS step must design a bounded client pathname transport, descriptor ownership tied to task identity, regular multi-byte reads and a controlled CPIO file index. General pathname resolution also requires directory handling, mount policy, credential checks, memory-safe client-copy conventions and error semantics before it can claim Linux `openat()` or `read()` compatibility. Writable filesystem, persistent block storage, cache coherency, `mmap` file mappings, `/proc`, `dpkg` and `apt` remain unimplemented.

## References

[1]: https://www.kernel.org/doc/Documentation/early-userspace/buffer-format.txt "Linux kernel initramfs buffer / newc CPIO format"
