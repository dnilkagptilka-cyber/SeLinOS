# SeLinOS ROMFS/VFS foundation M1

**Статус:** завершён и воспроизводимо проверен в QEMU TCG на x86_64/PC99.  
**Граница совместимости:** один immutable in-memory release record over isolated IPC; это не общий VFS и не Linux `open(2)`/`read(2)`/`close(2)` ABI.

ROMFS M1 заменяет placeholder `selinos-romfsd` отдельным server process и добавляет отдельный `selinos-romfs-probe`. Root allocates a single endpoint and копирует capability только в CSpaces этих двух processes. Fresh `sel4utils_configure_process` layout leaves copied cap in slot 8, and root rejects startup if either copy is not exactly this slot. Server and probe therefore receive no PCI, IRQ, DMA, device, raw VSpace or kernel-management capability from the ROMFS path.

| Operation | Request | Success reply | Verified guard |
|---|---|---|---|
| Open | `OPEN`, file ID `1` | fixed handle `3` | Any other file ID returns `ENOENT` |
| Read | `READ`, handle `3`, offset `0` | length `18`, immutable `SELINOS!` magic | Handle `99` returns `EBADF` |
| Close | `CLOSE`, handle `3` | status `OK` | Any other handle returns `EBADF` |

Payload transfer is deliberately by fixed message register values in M1, rather than a user pointer. This proves descriptor lifecycle and server/client IPC isolation while avoiding an unproven general cross-address-space copy facility in the VFS layer. The client validates immutable read content and the invalid-FD rejection before printing its success marker.

> **Capability boundary:** root performs setup only. After startup, `selinos-romfsd` receives requests directly on its endpoint and replies directly to the probe. The client cannot obtain any authority other than the endpoint that root copied into its CSpace.

## Reproducible verification

```bash
cd /home/ubuntu/helixos
./tools/verify_romfs_m1.py
```

The verifier rebuilds the image, hashes server, probe, protocol and root-wiring sources, checks endpoint allocation/copy and slot validation source contracts, then boots QEMU through PTY-backed serial capture. It requires:

```text
SeLinOS romfsd: immutable release record service online.
SeLinOS ROMFS probe: open/read/close and invalid-FD guard passed.
SeLinOS M0: twelve isolated service domains started.
```

The evidence record is [`tests/artifacts/selinos_romfs_m1.verification.json`](../tests/artifacts/selinos_romfs_m1.verification.json), and the latest trace is [`tests/artifacts/selinos_romfs_m1.boot.log`](../tests/artifacts/selinos_romfs_m1.boot.log).

## Not implemented

M1 has no CPIO parser, pathname handling, directories, arbitrary files, persistent storage, permissions, mounts, user-pointer reads, writes, data-page transfer, Linux syscall dispatch integration, dynamic linking or Linux ELF compatibility. It is a narrow, real isolated IPC substrate for the next VFS increment—not evidence for `apt`, `dpkg` or generic package file I/O.
