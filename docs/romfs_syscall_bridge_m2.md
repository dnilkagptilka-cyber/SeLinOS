# SeLinOS ROMFS syscall bridge M2

**Статус:** завершён и воспроизводимо проверен в QEMU TCG на x86_64/PC99.  
**Граница совместимости:** один fixed Linux-shaped `openat`/`read`/`close` sequence over the isolated ROMFS M1 server; не general Linux VFS ABI.

M2 связывает root-held Linux syscall fault gateway с direct IPC endpoint `selinos-romfsd`. Existing isolated syscall probe performs `openat(AT_FDCWD, "/selinos-release", 0, 0)`, receives fixed descriptor `3`, executes `read(3, mapped_page, 8)`, compares the returned bytes with `SELINOS!`, executes `close(3)`, then completes its existing EOF-read and `exit(0)` verification route.

| Linux-shaped operation | Accepted values | Server operation | Result |
|---|---|---|---|
| `openat` (257) | `dirfd=-100`, non-null pathname, flags/mode `0` | ROMFS `OPEN(file=1)` | descriptor `3` |
| `read` (0) | `fd=3`, non-null one-page mapped buffer, length `8` | ROMFS `READ(fd=3, offset=0)` | `SELINOS!` copied into caller page; result `8` |
| `close` (3) | `fd=3` | ROMFS `CLOSE(fd=3)` | result `0` |

Gateway takes a snapshot of the 16-register UnknownSyscall frame before issuing its own ROMFS IPC. It copies only the caller buffer’s backing frame cap, maps it temporarily in root, writes the fixed 8-byte immutable word, unmaps and deletes the duplicate mapping capability, restores the fault reply frame and resumes user code. Thus the gateway never treats a caller pointer as a root pointer and no standing shared mapping is granted.

> **End-to-end proof:** probe reaches the completion marker only after comparing `SELINOS!` in its own mapped VSpace and verifying `close(3) == 0`. Independent ROMFS probe also continues to prove direct server open/read/close and invalid-FD behavior in the same boot.

## Reproducible verification

```bash
cd /home/ubuntu/helixos
./tools/verify_romfs_syscall_bridge_m2.py
```

The verifier hashes the probe, gateway, ROMFS server and protocol sources, checks the temporary mapping contract, and requires a QEMU trace containing the mediated completion marker. The machine-readable record is [`tests/artifacts/selinos_romfs_syscall_bridge_m2.verification.json`](../tests/artifacts/selinos_romfs_syscall_bridge_m2.verification.json).

## Not implemented

M2 has no general path resolver, CPIO parser, directory layer, descriptor table, offset management, multi-page buffers, arbitrary data transfer, write, persistent storage, permissions, mount handling, `exec`, dynamic linking, glibc, `dpkg`, `apt` or existing Linux ELF application support. It is a narrow server-mediated syscall bridge, not a complete Linux file subsystem.
