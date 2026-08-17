# SeLinOS Phase 12 writable VFS checkpoint — 2026-08-14

The current default x86_64/PC99 QEMU-TCG image is:

```text
bedd595c46eab95b9e10b21b65d4620b4b533049ea1a033ac1f484e767d14c4e
```

## Verified VFS M0 scope

The default image now contains an isolated server-side **volatile VFS M0** transaction. It adds exactly one server-owned state record with the fixed packed pathname `/selinos-state` and fixed descriptor `5`. It is not a CPIO member, a block device abstraction or a persistent filesystem.

| Operation | Verified behavior | Evidence limit |
|---|---|---|
| Packed open | Exact 14-byte packed pathname opens the volatile descriptor. | No arbitrary path traversal, directory or multi-client descriptor table. |
| Initial read | The open descriptor at offset zero returns one fixed word. | No streaming or multi-page read. |
| Malformed write | A request without the complete payload is rejected and the prior state remains intact. | No partial writes or offset semantics. |
| Full-word write | One complete three-word IPC request atomically replaces the server-owned word, followed by verified read-back. | No append, mmap or shared client-memory transport. |
| Close | Close clears server-local open state; subsequent write is rejected. | No fork/exec inheritance or process cleanup semantics. |

The independent record `tests/artifacts/selinos_vfs_volatile_m0.verification.json` is verified by `tools/verify_vfs_volatile_m0.py`. It binds the image, server, client probe, endpoint protocol and serial log.

## Storage relationship

Virtio M0/M1/M2 remain separate default-OFF images. M2 observes one current feature window from QEMU’s modern virtio BAR4 through a root-only uncached read-only temporary mapping. It does not create a storage domain, queue, IRQ, DMA mapping, block request or persistence path. The VFS M0 word is therefore intentionally reinitialized at boot.

## Active blockers before persistent VFS or packages

| Blocker | Why VFS M0 does not solve it | Required later proof |
|---|---|---|
| No block backend | VFS M0 has no device access. | Virtio feature policy, queue lifecycle, IRQ/DMA containment and bounded block-I/O verifier. |
| No durability model | RAM state is lost at reboot. | Write ordering, atomic replace, flush, crash/recovery and corruption tests. |
| Minimal namespace | One fixed path is not a file hierarchy. | Bounded directories, metadata, permissions, symlinks and per-client FD ownership semantics. |
| Incomplete process ABI | Normal Linux programs cannot start. | TLS, clone/futex, signals, dynamic linker and W^X-safe executable mapping. |
| No package acquisition/trust | There is no network stack or trust policy. | NIC containment, IP/DNS/TCP/HTTPS, time, keyring and repository-verification evidence. |

> **Claim boundary:** VFS M0 is a deliberately narrow functional proof of validated volatile mutation within one isolated server. It does not claim a persistent filesystem, POSIX compatibility, `dpkg`, `apt`, or Linux package execution.

## Linux syscall ABI M6 update

The default image now also has independently verified ABI M6 evidence in `tools/verify_linux_syscall_abi_m6.py`. The fault-mediated probe accepts only `clock_gettime(CLOCK_MONOTONIC)` into its already mapped one-page test buffer and returns the fixed timespec `{1234, 0}`. It accepts only `getrlimit(RLIMIT_NOFILE)` at a fixed in-page offset and returns `{64, 64}`. These are deterministic fixture values rather than usable system-clock or resource-accounting semantics.

For the same mapped probe word, `FUTEX_WAIT` accepts only operation zero, expected value one and zero extra parameters, then returns `-EAGAIN` immediately. `FUTEX_WAKE` accepts only operation one and wake count one, then returns zero. No wait queue, blocking, timeout, PI, requeue, robust list, task scheduling or TLS state is introduced. The evidence records that the observed negative reply is checked through the x86-64 `EAX` return lane.

## Next bridge gate

`docs/phase12_linux_volatile_vfs_bridge_m1_gate.md` now defines the next M7 candidate: a Linux syscall bridge to the already verified volatile `/selinos-state` object. It is intentionally not implemented in the current image. The gate requires a purpose-named bounded pathname copier, exact NUL-terminated literal comparison, fixed eight-byte payload transport, one-page temporary root mapping teardown and malformed request/non-mutation evidence. The current M6 default image and its digest remain unchanged after this design work.

## Exact pathname-copy prerequisite update

The immutable Linux `openat` bridge now first duplicate-maps only the caller page, copies the complete NUL-terminated `/selinos-release` literal into root-local storage, compares every byte including the terminator, unmaps/deletes the duplicate capability, and only then sends its fixed server IPC. The M6 and ROMFS bridge verifiers passed after this hardening. This removes the former non-null-pointer-only acceptance condition; it remains a single-literal bridge rather than an arbitrary pathname facility.

## Linux-to-volatile-VFS M7 update

`tools/verify_linux_vfs_volatile_bridge_m7.py` now independently verifies a native Linux syscall bridge to the single server-owned volatile record. The probe first sends the exact non-matching `/selinos-statu` pathname and receives `-ENOENT` before the root sends a server `OPEN_PATH` IPC. It then sends only exact NUL-terminated `/selinos-state`, receives fixed FD 5, reads the reset word `BOOTM0!!`, writes only the fixed eight-byte `STATEM0!` word, reads it back, closes the FD and observes `-EBADF` for a subsequent fixed write.

Root copies pathnames and payload from one mapped client page into root-local memory, maps no device frame and transfers no client capability to the VFS server. The server resets its test word on each volatile-state open, deliberately preventing cross-probe state coupling. M7 remains a fixed, volatile test transaction: it provides neither persistence, arbitrary pathname processing, POSIX file semantics nor a package-manager prerequisite beyond this narrow bridge.
